package main

import (
	"github.com/sagernet/sing-box/log"
	"testing"
)

func TestParseConfig(t *testing.T) {
	if _, err := parseConfig([]byte(`{"inbounds":[],"outbounds":[]}`)); err != nil {
		t.Fatalf("valid config rejected: %v", err)
	}
	if _, err := parseConfig([]byte(`{"inbounds":`)); err == nil {
		t.Fatal("invalid config accepted")
	}
}

func TestDrainLogsClearsBoundedQueue(t *testing.T) {
	logBuffer.Lock()
	logBuffer.lines = nil
	logBuffer.Unlock()
	for index := 0; index < maxLogLines+10; index++ {
		bridgeLogWriter{}.WriteMessage(log.LevelInfo, "test")
	}
	logBuffer.Lock()
	count := len(logBuffer.lines)
	logBuffer.Unlock()
	if count != maxLogLines {
		t.Fatalf("log queue size = %d, want %d", count, maxLogLines)
	}
}
