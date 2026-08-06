package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"bytes"
	"fmt"
	"os"
	"strings"
	"sync"
	"unsafe"

	xraylog "github.com/xtls/xray-core/common/log"
	"github.com/xtls/xray-core/core"
	_ "github.com/xtls/xray-core/main/distro/all"
)

const (
	abiVersion  = 1
	maxLogLines = 512

	stateStopped  = 0
	stateStarting = 1
	stateRunning  = 2
	stateStopping = 3
	stateFailed   = 4
)

var runtimeState = struct {
	sync.Mutex
	instance *core.Instance
	state    int
}{state: stateStopped}

var logBuffer = struct {
	sync.Mutex
	lines []string
}{}

type bridgeLogHandler struct{}

func (bridgeLogHandler) Handle(message xraylog.Message) {
	if message == nil {
		return
	}
	line := strings.TrimSpace(message.String())
	if line == "" {
		return
	}
	logBuffer.Lock()
	defer logBuffer.Unlock()
	if len(logBuffer.lines) == maxLogLines {
		copy(logBuffer.lines, logBuffer.lines[1:])
		logBuffer.lines[len(logBuffer.lines)-1] = line
		return
	}
	logBuffer.lines = append(logBuffer.lines, line)
}

func installLogHandler() {
	xraylog.RegisterHandler(bridgeLogHandler{})
}

func cBytes(value *C.char, size C.size_t) ([]byte, error) {
	if value == nil {
		return nil, fmt.Errorf("config pointer is null")
	}
	const maxCInt = uint64(^uint32(0) >> 1)
	if uint64(size) > maxCInt {
		return nil, fmt.Errorf("config is too large")
	}
	return C.GoBytes(unsafe.Pointer(value), C.int(size)), nil
}

func assetPath(assetDir *C.char) (string, error) {
	if assetDir == nil {
		return "", fmt.Errorf("asset directory pointer is null")
	}
	path := C.GoString(assetDir)
	if path == "" {
		return "", fmt.Errorf("asset directory is empty")
	}
	return path, nil
}

func setAssetDirectory(path string) error {
	if path == "" {
		return fmt.Errorf("asset directory is empty")
	}
	return os.Setenv("xray.location.asset", path)
}

func resultString(err error) *C.char {
	if err == nil {
		return nil
	}
	return C.CString(err.Error())
}

func safeError(operation func() error) (err error) {
	defer func() {
		if recovered := recover(); recovered != nil {
			err = fmt.Errorf("embedded Xray panic: %v", recovered)
		}
	}()
	return operation()
}

func safeCall(operation func() error) *C.char {
	return resultString(safeError(operation))
}

func validateConfig(config []byte, assetDir string) error {
	if err := setAssetDirectory(assetDir); err != nil {
		return err
	}
	installLogHandler()
	parsed, err := core.LoadConfig("json", bytes.NewReader(config))
	if err != nil {
		return fmt.Errorf("config validation failed: %w", err)
	}
	instance, err := core.New(parsed)
	if err != nil {
		return fmt.Errorf("config validation failed: %w", err)
	}
	if err := instance.Close(); err != nil {
		return fmt.Errorf("config validation cleanup failed: %w", err)
	}
	return nil
}

func validateRequest(config []byte, assetDir string) error {
	runtimeState.Lock()
	defer runtimeState.Unlock()
	if runtimeState.instance != nil {
		return fmt.Errorf("validation is unavailable while the main Xray instance is running")
	}
	return validateConfig(config, assetDir)
}

func startRequest(config []byte, assetDir string) (err error) {
	runtimeState.Lock()
	defer runtimeState.Unlock()
	defer func() {
		if recovered := recover(); recovered != nil {
			runtimeState.state = stateFailed
			err = fmt.Errorf("start panic: %v", recovered)
		}
	}()
	if runtimeState.instance != nil {
		return fmt.Errorf("an Xray instance is already running")
	}
	if err := setAssetDirectory(assetDir); err != nil {
		return err
	}
	runtimeState.state = stateStarting
	installLogHandler()
	instance, err := core.StartInstance("json", config)
	if err != nil {
		runtimeState.state = stateFailed
		return fmt.Errorf("start failed: %w", err)
	}
	runtimeState.instance = instance
	runtimeState.state = stateRunning
	installLogHandler()
	return nil
}

func stopRequest() (err error) {
	runtimeState.Lock()
	defer runtimeState.Unlock()
	defer func() {
		if recovered := recover(); recovered != nil {
			runtimeState.state = stateFailed
			err = fmt.Errorf("stop panic: %v", recovered)
		}
	}()
	if runtimeState.instance == nil {
		runtimeState.state = stateStopped
		return nil
	}
	runtimeState.state = stateStopping
	instance := runtimeState.instance
	runtimeState.instance = nil
	if err := instance.Close(); err != nil {
		runtimeState.state = stateFailed
		return fmt.Errorf("stop failed: %w", err)
	}
	runtimeState.state = stateStopped
	return nil
}

//export ZaryaXrayAbiVersion
func ZaryaXrayAbiVersion() (result C.int) {
	defer func() {
		if recover() != nil {
			result = 0
		}
	}()
	return abiVersion
}

//export ZaryaXrayVersion
func ZaryaXrayVersion() (result *C.char) {
	defer func() {
		if recovered := recover(); recovered != nil {
			result = C.CString(fmt.Sprintf("version unavailable: %v", recovered))
		}
	}()
	return C.CString(core.Version())
}

//export ZaryaXrayValidate
func ZaryaXrayValidate(configJSON *C.char, configSize C.size_t, assetDir *C.char) *C.char {
	return safeCall(func() error {
		config, err := cBytes(configJSON, configSize)
		if err != nil {
			return err
		}
		path, err := assetPath(assetDir)
		if err != nil {
			return err
		}
		return validateRequest(config, path)
	})
}

//export ZaryaXrayStart
func ZaryaXrayStart(configJSON *C.char, configSize C.size_t, assetDir *C.char) *C.char {
	return safeCall(func() error {
		config, err := cBytes(configJSON, configSize)
		if err != nil {
			return err
		}
		path, err := assetPath(assetDir)
		if err != nil {
			return err
		}
		return startRequest(config, path)
	})
}

//export ZaryaXrayStop
func ZaryaXrayStop() *C.char {
	return safeCall(stopRequest)
}

//export ZaryaXrayState
func ZaryaXrayState() (result C.int) {
	defer func() {
		if recover() != nil {
			result = stateFailed
		}
	}()
	runtimeState.Lock()
	defer runtimeState.Unlock()
	if runtimeState.instance != nil && !runtimeState.instance.IsRunning() {
		runtimeState.state = stateFailed
	}
	return C.int(runtimeState.state)
}

//export ZaryaXrayDrainLogs
func ZaryaXrayDrainLogs() (result *C.char) {
	defer func() {
		if recovered := recover(); recovered != nil {
			result = C.CString(fmt.Sprintf("embedded Xray log panic: %v", recovered))
		}
	}()
	logBuffer.Lock()
	defer logBuffer.Unlock()
	joined := strings.Join(logBuffer.lines, "\n")
	logBuffer.lines = nil
	return C.CString(joined)
}

//export ZaryaXrayFree
func ZaryaXrayFree(value unsafe.Pointer) {
	defer func() { _ = recover() }()
	if value != nil {
		C.free(value)
	}
}

func main() {}
