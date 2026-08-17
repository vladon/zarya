#!/usr/bin/env python3
"""Contract tests for SignPath release signing and fail-closed verification."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parents[1]


def load_verifier():
    path = ROOT / "scripts" / "verify-release-artifacts.py"
    spec = importlib.util.spec_from_file_location("zarya_verify_release", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


VERIFIER = load_verifier()


class ReleaseSigningContractTest(unittest.TestCase):
    def make_windows_staging(self, root: Path) -> dict:
        artifacts = {
            "gui": "Zarya.exe",
            "helper": "zarya-helper.exe",
            "updater": "zarya-updater.exe",
            "coreTestWorker": "zarya-core-test-worker.exe",
            "xrayBridge": "zarya-xray.dll",
        }
        for name in artifacts.values():
            (root / name).write_bytes(name.encode("ascii"))
        return {"platform": "windows", "artifacts": artifacts}

    def test_windows_verification_requires_every_declared_executable(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            manifest = self.make_windows_staging(root)
            completed = SimpleNamespace(returncode=0, stdout="", stderr="")
            with mock.patch.object(VERIFIER, "_find_signtool", return_value="signtool"), mock.patch.object(
                VERIFIER.subprocess, "run", return_value=completed
            ) as run:
                valid, details = VERIFIER.verify_windows_signatures(root, manifest)
            self.assertTrue(valid, details)
            self.assertEqual(run.call_count, 5)
            for call in run.call_args_list:
                self.assertIn("/all", call.args[0])
                self.assertIn("/tw", call.args[0])

            (root / "zarya-updater.exe").unlink()
            valid, details = VERIFIER.verify_windows_signatures(root, manifest)
            self.assertFalse(valid)
            self.assertEqual(details["windowsAuthenticode"]["zarya-updater.exe"], "missing")

    def test_one_invalid_signature_fails_the_package(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            manifest = self.make_windows_staging(root)

            def verify(args, **_kwargs):
                return SimpleNamespace(
                    returncode=1 if str(args[-1]).endswith("zarya-updater.exe") else 0,
                    stdout="",
                    stderr="",
                )

            with mock.patch.object(VERIFIER, "_find_signtool", return_value="signtool"), mock.patch.object(
                VERIFIER.subprocess, "run", side_effect=verify
            ):
                valid, details = VERIFIER.verify_windows_signatures(root, manifest)
            self.assertFalse(valid)
            self.assertEqual(
                details["windowsAuthenticode"]["zarya-updater.exe"],
                "unsigned or invalid",
            )

    def test_external_signing_refreshes_manifest_checksums(self) -> None:
        import sys

        sys.path.insert(0, str(ROOT / "scripts"))
        from release_common import refresh_manifest_checksums

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            exe = root / "Zarya.exe"
            exe.write_bytes(b"signed payload")
            manifest = {
                "checksums": {
                    "Zarya.exe": "sha256:stale",
                    "cores/xray/xray.exe": "sha256:unchanged",
                }
            }
            (root / "release-manifest.json").write_text(
                json.dumps(manifest), encoding="utf-8"
            )
            refresh_manifest_checksums(root)
            updated = json.loads((root / "release-manifest.json").read_text(encoding="utf-8"))
            expected = hashlib.sha256(b"signed payload").hexdigest()
            self.assertEqual(updated["checksums"]["Zarya.exe"], f"sha256:{expected}")
            self.assertEqual(
                updated["checksums"]["cores/xray/xray.exe"], "sha256:unchanged"
            )

    def test_signpath_configuration_signs_only_zarya_exe(self) -> None:
        config = ROOT / ".signpath" / "artifact-configuration.xml"
        tree = ElementTree.parse(config)
        namespace = {"s": "http://signpath.io/artifact-configuration/v1"}
        files = tree.findall(".//s:pe-file", namespace)
        self.assertEqual(
            {item.attrib["path"] for item in files},
            {"Zarya.exe"},
        )
        for item in files:
            self.assertEqual(item.attrib["product-name"], "Zarya")
            self.assertEqual(item.attrib["product-version"], "${version}")
            self.assertEqual(item.attrib["company-name"], "vladon.dev")
            signature = item.find("s:authenticode-sign", namespace)
            self.assertIsNotNone(signature)
            self.assertEqual(signature.attrib["hash-algorithm"], "sha256")

    def test_release_workflow_is_gated_and_fail_closed(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "release.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn("environment: release-signing", workflow)
        self.assertIn(
            "signpath/github-action-submit-signing-request@b9d91eadd323de506c0c81cf0c7fe7438f3360fd",
            workflow,
        )
        self.assertIn("github-artifact-id:", workflow)
        self.assertIn("wait-for-completion: true", workflow)
        self.assertIn("finalize-signpath-windows.ps1", workflow)
        self.assertNotIn("continue-on-error", workflow)
        finalizer = (ROOT / "scripts" / "finalize-signpath-windows.ps1").read_text(
            encoding="utf-8"
        )
        self.assertIn("--require-signed", finalizer)
        self.assertIn("--windows-lcl-single-exe", finalizer)
        self.assertIn("SignPath Foundation", finalizer)
        # Fail closed: the finalizer must not warn its way past a bad package.
        self.assertNotIn("Write-Warning", finalizer)
        # The Qt-era multi-file smoke gate (LICENSE, helper, updater, bridges)
        # does not apply to the flat LCL single-EXE ZIP and must not run there.
        self.assertNotIn('"scripts\\smoke-package.py"', finalizer)

    def test_public_policy_contains_foundation_requirements(self) -> None:
        policy = (ROOT / "docs" / "signing" / "code-signing-policy.md").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            "Free code signing provided by [SignPath.io]", policy
        )
        self.assertIn("certificate by\n[SignPath Foundation]", policy)
        self.assertIn("Committers and reviewers", policy)
        self.assertIn("Signing approvers", policy)
        self.assertIn("Privacy", policy)


def make_lcl_zip(temp: Path, name: str, files: dict[str, bytes]) -> Path:
    import zipfile

    artifact = temp / name
    with zipfile.ZipFile(artifact, "w", zipfile.ZIP_DEFLATED) as archive:
        for entry, payload in files.items():
            archive.writestr(entry, payload)
    return artifact


class WindowsLclSingleExeContractTest(unittest.TestCase):
    """Negative packaging/signing tests for the LCL single-EXE ZIP."""

    GOOD_EXE = b"MZ" + b"\x00" * 64

    def setUp(self) -> None:
        import hashlib

        self._temp = tempfile.TemporaryDirectory()
        self.temp = Path(self._temp.name)
        self.good_digest = hashlib.sha256(self.GOOD_EXE).hexdigest()

    def tearDown(self) -> None:
        self._temp.cleanup()

    def extract(self, artifact: Path) -> Path:
        dest = self.temp / "extracted"
        VERIFIER.extract_zip(artifact, dest)
        return dest

    def good_zip(self) -> Path:
        return make_lcl_zip(
            self.temp,
            "good.zip",
            {
                "Zarya.exe": self.GOOD_EXE,
                "Zarya.exe.sha256": f"{self.good_digest} *Zarya.exe\n".encode(),
            },
        )

    def test_valid_two_file_zip_passes(self) -> None:
        artifact = self.good_zip()
        with mock.patch.object(
            VERIFIER,
            "run_version_check",
            return_value=(True, "Zarya 1.5.13"),
        ):
            errors = VERIFIER.verify_windows_lcl_single_exe(
                artifact, self.extract(artifact), "1.5.13"
            )
        self.assertEqual(errors, [])

    def test_zip_with_extra_file_fails(self) -> None:
        artifact = make_lcl_zip(
            self.temp,
            "extra.zip",
            {
                "Zarya.exe": self.GOOD_EXE,
                "Zarya.exe.sha256": f"{self.good_digest} *Zarya.exe\n".encode(),
                "extra.dll": b"dll payload",
            },
        )
        errors = VERIFIER.verify_windows_lcl_single_exe(
            artifact, self.extract(artifact)
        )
        self.assertTrue(errors)
        self.assertTrue(
            any("extra.dll" in error for error in errors), errors
        )

    def test_zip_with_dll_renamed_as_exe_partner_fails(self) -> None:
        artifact = make_lcl_zip(
            self.temp,
            "nested.zip",
            {
                "Zarya.exe": self.GOOD_EXE,
                "Zarya.exe.sha256": f"{self.good_digest} *Zarya.exe\n".encode(),
                "lib/zarya-xray.dll": b"dll payload",
            },
        )
        errors = VERIFIER.verify_windows_lcl_single_exe(
            artifact, self.extract(artifact)
        )
        self.assertTrue(
            any("nested directory" in error for error in errors), errors
        )

    def test_zip_without_sha256_sidecar_fails(self) -> None:
        artifact = make_lcl_zip(
            self.temp, "no-sha.zip", {"Zarya.exe": self.GOOD_EXE}
        )
        errors = VERIFIER.verify_windows_lcl_single_exe(
            artifact, self.extract(artifact)
        )
        self.assertTrue(errors)
        self.assertTrue(
            any("Zarya.exe.sha256" in error for error in errors), errors
        )

    def test_mismatched_sha256_fails(self) -> None:
        artifact = make_lcl_zip(
            self.temp,
            "bad-sha.zip",
            {
                "Zarya.exe": self.GOOD_EXE,
                "Zarya.exe.sha256": f"{'0' * 64} *Zarya.exe\n".encode(),
            },
        )
        errors = VERIFIER.verify_windows_lcl_single_exe(
            artifact, self.extract(artifact)
        )
        self.assertTrue(
            any("does not match" in error for error in errors), errors
        )

    def _run_main_require_signed(self, artifact: Path, signtool_result):
        import sys

        argv = [
            "verify-release-artifacts.py",
            "--artifact",
            str(artifact),
            "--windows-lcl-single-exe",
            "--require-signed",
        ]
        with mock.patch.object(
            VERIFIER, "_find_signtool", return_value="signtool"
        ), mock.patch.object(
            VERIFIER.subprocess, "run", return_value=signtool_result
        ) as run, mock.patch.object(
            VERIFIER,
            "run_version_check",
            return_value=(True, "Zarya 1.5.13"),
        ), mock.patch.object(sys, "argv", argv):
            return VERIFIER.main(), run

    def test_unsigned_exe_fails_require_signed_mode(self) -> None:
        artifact = self.good_zip()
        unsigned = SimpleNamespace(returncode=1, stdout="", stderr="")
        exit_code, run = self._run_main_require_signed(artifact, unsigned)
        self.assertEqual(exit_code, 1)
        self.assertEqual(run.call_count, 1)
        self.assertIn("Zarya.exe", str(run.call_args.args[0]))

    def test_tampered_exe_fails_require_signed_mode(self) -> None:
        # A signature over a different payload must fail closed even when the
        # ZIP layout itself is valid: signtool rejects the modified EXE.
        payload = self.GOOD_EXE + b"tampered"
        import hashlib

        artifact = make_lcl_zip(
            self.temp,
            "tampered.zip",
            {
                "Zarya.exe": payload,
                "Zarya.exe.sha256": (
                    f"{hashlib.sha256(payload).hexdigest()} *Zarya.exe\n".encode()
                ),
            },
        )
        broken = SimpleNamespace(returncode=1, stdout="", stderr="")
        exit_code, _run = self._run_main_require_signed(artifact, broken)
        self.assertEqual(exit_code, 1)




if __name__ == "__main__":
    unittest.main()
