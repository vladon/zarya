package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"unsafe"

	box "github.com/sagernet/sing-box"
	constant "github.com/sagernet/sing-box/constant"
	"github.com/sagernet/sing-box/log"
	"github.com/sagernet/sing-box/option"
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
	instance *box.Box
	state    int
}{state: stateStopped}

var logBuffer = struct {
	sync.Mutex
	lines []string
}{}

type bridgeLogWriter struct{}

func (bridgeLogWriter) WriteMessage(level log.Level, message string) {
	line := strings.TrimSpace(fmt.Sprintf("%s: %s", log.FormatLevel(level), message))
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

func resultString(err error) *C.char {
	if err == nil {
		return nil
	}
	return C.CString(err.Error())
}

func safeError(operation func() error) (err error) {
	defer func() {
		if recovered := recover(); recovered != nil {
			err = fmt.Errorf("embedded sing-box panic: %v", recovered)
		}
	}()
	return operation()
}

func safeCall(operation func() error) *C.char {
	return resultString(safeError(operation))
}

func parseConfig(config []byte) (option.Options, error) {
	var options option.Options
	if err := options.UnmarshalJSONContext(context.Background(), config); err != nil {
		return option.Options{}, fmt.Errorf("config validation failed: %w", err)
	}
	return options, nil
}

func validateRequest(config []byte) error {
	runtimeState.Lock()
	defer runtimeState.Unlock()
	if runtimeState.instance != nil {
		return fmt.Errorf("validation is unavailable while sing-box is running")
	}
	options, err := parseConfig(config)
	if err != nil {
		return err
	}
	instance, err := box.New(box.Options{Options: options, Context: context.Background(), PlatformLogWriter: bridgeLogWriter{}})
	if err != nil {
		return fmt.Errorf("config validation failed: %w", err)
	}
	if err := instance.Close(); err != nil {
		return fmt.Errorf("config validation cleanup failed: %w", err)
	}
	return nil
}

func startRequest(config []byte) (err error) {
	runtimeState.Lock()
	defer runtimeState.Unlock()
	defer func() {
		if recovered := recover(); recovered != nil {
			runtimeState.state = stateFailed
			err = fmt.Errorf("start panic: %v", recovered)
		}
	}()
	if runtimeState.instance != nil {
		return fmt.Errorf("a sing-box instance is already running")
	}
	runtimeState.state = stateStarting
	options, err := parseConfig(config)
	if err != nil {
		runtimeState.state = stateFailed
		return err
	}
	instance, err := box.New(box.Options{Options: options, Context: context.Background(), PlatformLogWriter: bridgeLogWriter{}})
	if err != nil {
		runtimeState.state = stateFailed
		return fmt.Errorf("start failed: %w", err)
	}
	if err := instance.Start(); err != nil {
		_ = instance.Close()
		runtimeState.state = stateFailed
		return fmt.Errorf("start failed: %w", err)
	}
	runtimeState.instance = instance
	runtimeState.state = stateRunning
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

//export ZaryaSingBoxAbiVersion
func ZaryaSingBoxAbiVersion() (result C.int) {
	defer func() {
		if recover() != nil {
			result = 0
		}
	}()
	return abiVersion
}

//export ZaryaSingBoxVersion
func ZaryaSingBoxVersion() *C.char {
	return C.CString(constant.Version)
}

//export ZaryaSingBoxValidate
func ZaryaSingBoxValidate(configJSON *C.char, configSize C.size_t) *C.char {
	return safeCall(func() error {
		config, err := cBytes(configJSON, configSize)
		if err != nil {
			return err
		}
		return validateRequest(config)
	})
}

//export ZaryaSingBoxStart
func ZaryaSingBoxStart(configJSON *C.char, configSize C.size_t) *C.char {
	return safeCall(func() error {
		config, err := cBytes(configJSON, configSize)
		if err != nil {
			return err
		}
		return startRequest(config)
	})
}

//export ZaryaSingBoxStop
func ZaryaSingBoxStop() *C.char { return safeCall(stopRequest) }

//export ZaryaSingBoxState
func ZaryaSingBoxState() (result C.int) {
	defer func() {
		if recover() != nil {
			result = stateFailed
		}
	}()
	runtimeState.Lock()
	defer runtimeState.Unlock()
	return C.int(runtimeState.state)
}

//export ZaryaSingBoxDrainLogs
func ZaryaSingBoxDrainLogs() *C.char {
	logBuffer.Lock()
	defer logBuffer.Unlock()
	joined := strings.Join(logBuffer.lines, "\n")
	logBuffer.lines = nil
	return C.CString(joined)
}

//export ZaryaSingBoxFree
func ZaryaSingBoxFree(value unsafe.Pointer) {
	if value != nil {
		C.free(value)
	}
}

func init() { constant.Version = "1.13.12" }

func main() {}
