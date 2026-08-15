package main

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"unsafe"

	xraylog "github.com/xtls/xray-core/common/log"
)

func TestLogQueueIsBounded(t *testing.T) {
	logBuffer.Lock()
	logBuffer.lines = nil
	logBuffer.Unlock()

	handler := bridgeLogHandler{}
	for index := 0; index < maxLogLines+10; index++ {
		handler.Handle(&xraylog.GeneralMessage{Content: index})
	}
	logBuffer.Lock()
	defer logBuffer.Unlock()
	if got := len(logBuffer.lines); got != maxLogLines {
		t.Fatalf("log queue length = %d, want %d", got, maxLogLines)
	}
}

func TestHTTPProxyProbe(t *testing.T) {
	proxyServer := httptest.NewServer(http.HandlerFunc(func(writer http.ResponseWriter,
		request *http.Request) {
		if request.URL.String() != "http://probe.invalid/generate_204" {
			t.Errorf("proxy request URL = %q", request.URL.String())
		}
		writer.WriteHeader(http.StatusNoContent)
	}))
	defer proxyServer.Close()

	proxyURL := strings.TrimPrefix(proxyServer.URL, "http://")
	host, port, found := strings.Cut(proxyURL, ":")
	if !found {
		t.Fatalf("unexpected proxy URL %q", proxyServer.URL)
	}
	delay, err := probeURL("http://probe.invalid/generate_204", "mixed", host,
		mustAtoi(t, port), 3000)
	if err != nil {
		t.Fatalf("HTTP proxy probe failed: %v", err)
	}
	if delay < 0 {
		t.Fatalf("HTTP proxy delay = %d", delay)
	}
}

func TestProbeRejectsUnsafeInput(t *testing.T) {
	if _, err := probeURL("file:///secret", "mixed", "127.0.0.1", 1080, 1000); err == nil {
		t.Fatal("file URL was accepted")
	}
	if _, err := probeURL("https://example.com", "direct", "127.0.0.1", 1080, 1000); err == nil {
		t.Fatal("unsupported proxy kind was accepted")
	}
}

func mustAtoi(t *testing.T, value string) int {
	t.Helper()
	result := 0
	for _, char := range value {
		if char < '0' || char > '9' {
			t.Fatalf("invalid integer %q", value)
		}
		result = result*10 + int(char-'0')
	}
	return result
}

func TestStopIsIdempotent(t *testing.T) {
	runtimeState.Lock()
	runtimeState.instance = nil
	runtimeState.state = stateStopped
	runtimeState.Unlock()
	if result := ZaryaXrayStop(); result != nil {
		defer ZaryaXrayFree(unsafe.Pointer(result))
		t.Fatal("first stop failed")
	}
	if result := ZaryaXrayStop(); result != nil {
		defer ZaryaXrayFree(unsafe.Pointer(result))
		t.Fatal("second stop failed")
	}
}

func TestAbiVersion(t *testing.T) {
	if got := int(ZaryaXrayAbiVersion()); got != abiVersion {
		t.Fatalf("ABI version = %d, want %d", got, abiVersion)
	}
}

func TestValidateStartStopRestart(t *testing.T) {
	assetDir := t.TempDir()
	config := []byte("{\"log\":{\"loglevel\":\"none\"},\"outbounds\":[{\"protocol\":\"freedom\",\"tag\":\"direct\"}]}")
	if err := validateRequest([]byte("{invalid-json}"), assetDir); err == nil {
		t.Fatal("invalid config passed validation")
	}
	if err := validateRequest(config, assetDir); err != nil {
		t.Fatalf("valid config failed validation: %v", err)
	}

	for cycle := 0; cycle < 3; cycle++ {
		if err := startRequest(config, assetDir); err != nil {
			t.Fatalf("cycle %d start failed: %v", cycle, err)
		}
		if err := startRequest(config, assetDir); err == nil {
			t.Fatalf("cycle %d double start unexpectedly succeeded", cycle)
		}
		runtimeState.Lock()
		running := runtimeState.instance != nil && runtimeState.instance.IsRunning()
		runtimeState.Unlock()
		if !running {
			t.Fatalf("cycle %d instance is not running", cycle)
		}
		if err := stopRequest(); err != nil {
			t.Fatalf("cycle %d stop failed: %v", cycle, err)
		}
		if err := stopRequest(); err != nil {
			t.Fatalf("cycle %d repeated stop failed: %v", cycle, err)
		}
	}
}

func TestSafeErrorRecoversPanic(t *testing.T) {
	err := safeError(func() error {
		panic("bridge-test-panic")
	})
	if err == nil || !strings.Contains(err.Error(), "bridge-test-panic") {
		t.Fatalf("panic was not recovered: %v", err)
	}
}

func TestVersionMemoryCanBeFreed(t *testing.T) {
	value := ZaryaXrayVersion()
	if value == nil {
		t.Fatal("version allocation returned nil")
	}
	ZaryaXrayFree(unsafe.Pointer(value))
}
