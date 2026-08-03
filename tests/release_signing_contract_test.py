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
            self.assertEqual(run.call_count, 3)
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

    def test_signpath_configuration_signs_only_zarya_binaries(self) -> None:
        config = ROOT / ".signpath" / "artifact-configuration.xml"
        tree = ElementTree.parse(config)
        namespace = {"s": "http://signpath.io/artifact-configuration/v1"}
        files = tree.findall(".//s:pe-file", namespace)
        self.assertEqual(
            {item.attrib["path"] for item in files},
            {"Zarya.exe", "zarya-helper.exe", "zarya-updater.exe"},
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
        self.assertIn("SignPath Foundation", finalizer)

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


if __name__ == "__main__":
    unittest.main()
