# name=FLRP Script  
# device=FLRP.py

import time
import json
import os
import arrangement
import channels
import device
import general
import launchMapPages
import mixer
import patterns
import playlist
import plugins
import screen
import transport
import ui
import midi
import utils
import subprocess

class MidiControllerConfig:
    def __init__(self):
        self._PossibleStates = ['Idle', 'Recording', 'Listening', 'Composing']
        self._ActiveState = self._PossibleStates[0]
        self._BPM = 0
        self.sync_interval = 0.5
        self._ActivePlugin = ""
        self.last_sync_time = time.time()
        self.session_start_time = int(time.time())
        
        # Simplified approach - just use the default location that we know works
        self.state_file_path = "fl_studio_state.json"  # FL Studio script directory
        self.tried_paths = []
        
        print("FL Studio Rich Presence instance created.")
        print(f"State file: {self.state_file_path}")
        
        # Test if we can actually write (but don't fail if we can't during init)
        try:
            self._write_state_file()
            print("✓ Initial write test successful")
        except Exception as e:
            print(f"⚠️ Initial write test failed: {e} (will retry during operation)")

    def _test_write_location(self, path):
        """Test if we can write to this location"""
        try:
            # Create directory if needed
            dir_path = os.path.dirname(path)
            if dir_path and not os.path.exists(dir_path):
                try:
                    os.makedirs(dir_path, exist_ok=True)
                except:
                    pass  # Ignore directory creation errors
            
            # Simple write test
            test_data = {"test": True}
            with open(path, 'w', encoding='utf-8') as f:
                json.dump(test_data, f)
            
            # Test reading it back
            with open(path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # Clean up test file
            try:
                if os.path.exists(path):
                    os.remove(path)
            except:
                pass  # Ignore cleanup errors
            
            print(f"✓ Can write to: {path}")
            return True
            
        except Exception as e:
            print(f"✗ Cannot write to {path}: {str(e)}")
            self.tried_paths.append(f"{path} ({str(e)})")
            return False

    def _write_state_file(self):
        """Write current state to JSON file"""
        if not self.state_file_path:
            print("❌ No valid state file path!")
            return
            
        state_data = {
            "state": self._ActiveState,
            "bpm": self._BPM,
            "plugin": self._ActivePlugin,
            "timestamp": self.session_start_time,
            "project_name": general.getProjectTitle(),  # Track current project
            "write_time": int(time.time())  # Track when file was written
        }
        
        max_retries = 3
        for attempt in range(max_retries):
            try:
                # Get absolute path to ensure we're writing to the right place
                abs_path = os.path.abspath(self.state_file_path)
                
                # Write with explicit encoding and flushing
                with open(abs_path, 'w', encoding='utf-8') as f:
                    json.dump(state_data, f, indent=2)
                    f.flush()  # Force write to disk
                    os.fsync(f.fileno())  # Force OS to write to disk
                
                # Verify the write worked by reading it back
                with open(abs_path, 'r', encoding='utf-8') as f:
                    verify_data = json.load(f)
                    if verify_data.get('write_time') == state_data['write_time']:
                        print(f"✅ Verified write: {self._ActiveState} @ {self._BPM}bpm to {abs_path}")
                        return  # Success!
                    else:
                        raise Exception("Write verification failed")
                        
            except Exception as e:
                print(f"❌ Write attempt {attempt + 1} failed: {e}")
                if attempt < max_retries - 1:
                    # Try to reinitialize the file path
                    time.sleep(0.1)
                    self._reinitialize_file_path()
                else:
                    print(f"❌ All write attempts failed for: {self.state_file_path}")
    
    def _reinitialize_file_path(self):
        """Reinitialize file path - useful when FL Studio changes projects"""
        print("🔄 Reinitializing file path...")
        old_path = self.state_file_path
        
        # Test locations again
        test_locations = [
            os.path.join(os.environ.get('TEMP', 'C:\\temp'), "fl_studio_state.json"),
            "C:\\temp\\fl_studio_state.json",
            os.path.join(os.path.expanduser("~"), "fl_studio_state.json"),
            "fl_studio_state.json"
        ]
        
        for path in test_locations:
            if self._test_write_location(path):
                if path != old_path:
                    print(f"🔄 Switched from {old_path} to {path}")
                self.state_file_path = path
                return
        
        print("⚠️ Could not find any writable location during reinitialize!")

    def _Sync(self):
        self.getBPM()
        old_state = self._ActiveState
        old_plugin = self._ActivePlugin
        old_bpm = getattr(self, '_last_bpm', 0)
        
        self.getState()
        
        # Update plugin info when idle
        if self._ActiveState == self._PossibleStates[0] and self.getFocusedPlugin() != "":
            self._ActivePlugin = self.getFocusedPlugin()
        
        # Write state file if anything changed OR every 5 seconds to keep it fresh
        current_time = time.time()
        force_update = (current_time - getattr(self, '_last_file_write', 0)) > 5.0
        
        if (old_state != self._ActiveState or 
            old_plugin != self._ActivePlugin or 
            abs(self._BPM - old_bpm) > 0.1 or
            force_update):
            
            self._write_state_file()
            self._last_bpm = self._BPM
            self._last_file_write = current_time
            
            if old_state != self._ActiveState:
                print(f"🔄 State changed to: {self._ActiveState}, BPM: {self._BPM}")
            elif force_update:
                print(f"🔄 Periodic update: {self._ActiveState} @ {self._BPM}bpm")

    def getBPM(self):
        self._BPM = mixer.getCurrentTempo(1) 
        
    def getFocusedPlugin(self):
        return ui.getFocusedPluginName()

    def getState(self):
        if transport.isRecording():
            self._ActiveState = self._PossibleStates[1]
        elif ui.getFocused(3):  # Piano Roll
            self._ActiveState = self._PossibleStates[3]
        elif transport.isPlaying():
            self._ActiveState = self._PossibleStates[2]
        else:  # default idle
            self._ActiveState = self._PossibleStates[0]


def OnInit():
    device.createRefreshThread()
    # Write initial state
    _controller._write_state_file()

    print("🎵 FLRP script initialized and ready!")

def OnProjectLoad(status):
    """Called when project is loaded - reinitialize file path"""
    print(f"🎼 Project loaded (status: {status})")
    _controller._reinitialize_file_path()
    _controller._write_state_file()  # Write state immediately after project load

def OnDeInit():
    # Clean up state file on exit
    try:
        if _controller.state_file_path and os.path.exists(_controller.state_file_path):
            os.remove(_controller.state_file_path)
            print("🧹 State file cleaned up.")
    except:
        pass
    return
            
def OnIdle():
    current_time = time.time()
    if current_time - _controller.last_sync_time > _controller.sync_interval:
        _controller.last_sync_time = current_time
        _controller._Sync()
        # Less verbose output
        if _controller._ActiveState != "Idle" or _controller._ActivePlugin:
            print(f"📊 {_controller._ActiveState} | {_controller._BPM}bpm | {_controller._ActivePlugin}")

        
_controller = MidiControllerConfig()