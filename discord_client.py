"""
FL Studio Discord Rich Presence Client
Reads state from FL Studio and updates Discord
"""

import json
import time
import os
import psutil
from pathlib import Path

try:
    from pypresence import Presence
    DISCORD_AVAILABLE = True
except ImportError:
    print("pypresence not installed!")
    print("Install with: pip install pypresence")
    DISCORD_AVAILABLE = False
    exit(1)

try:
    from dotenv import load_dotenv
    DOTENV_AVAILABLE = True
except ImportError:
    print("⚠️ python-dotenv not installed (optional)")
    print("Install with: pip install python-dotenv")
    DOTENV_AVAILABLE = False

class FLStudioDiscordClient:
    def __init__(self):
        # Load environment variables
        if DOTENV_AVAILABLE:
            load_dotenv()
        
        # Get Discord Application ID from environment or use default
        self.client_id = os.getenv('DISCORD_APPLICATION_ID', '123456789012345678')
        self.state_file_path = os.getenv('STATE_FILE_PATH',None)
        
        # Validate Application ID
        if self.client_id == '123456789012345678':
            print("Using default Discord Application ID!")
            print("Please create a .env file with your Discord Application ID:")
            print("DISCORD_APPLICATION_ID=your_actual_application_id_here")
            print("Or set it as an environment variable.")
            print()
        
        if self.state_file_path == None:
            print('State file not found')
            exit(1)
    
        
        # Discord connection
        self.rpc = None
        self.connected = False
        self.last_state = {}
        
        print("🎵 FL Studio Discord Rich Presence Client")
        print(f"Discord App ID: {self.client_id}")
        print(f"Monitoring: {self.state_file_path}")
        print()
        
    def connect_discord(self):
        """Connect to Discord Rich Presence"""
        try:
            self.rpc = Presence(self.client_id)
            self.rpc.connect()
            self.connected = True
            print("Connected to Discord")
            return True
        except Exception as e:
            print(f"Failed to connect to Discord: {e}")
            print("Make sure Discord is running and your Application ID is correct")
            return False
    
    def read_fl_state(self):
        """Read FL Studio state from JSON file"""
        try:
            if not os.path.exists(self.state_file_path):
                return None
                
            with open(self.state_file_path, 'r') as f:
                data = json.load(f)
                
            return {
                'state': data.get('state', 'Unknown'),
                'bpm': data.get('bpm', 120),
                'plugin': data.get('plugin', ''),
                'timestamp': data.get('timestamp', int(time.time()))
            }
        except Exception as e:
            print(f"Error reading state file: {e}")
            return None
    
    def update_discord_presence(self, fl_state):
        """Update Discord Rich Presence with FL Studio state"""
        if not self.connected or not self.rpc:
            return
            
        try:
            
            # Note: you will need to upload some icons
            
            # Base data
            presence_data = {
                "large_image": "fl_studio_logo",  
                "large_text": "FL Studio",
                "start": fl_state['timestamp']
            }
            
            state = fl_state['state']
            bpm = fl_state['bpm']
            plugin = fl_state['plugin']
            presence_data["state"] = f"{bpm} BPM"
            
            # States
            
            if state == "Recording":
                presence_data.update({
                    "details": "Recording",  
                    "small_image": "recording",
                    "small_text": "Recording"
                })
                
            elif state == "Listening":
                presence_data.update({
                    "details": "Listening to Track",  
                    "small_image": "listening",
                    "small_text": "Playing"
                })
                
            elif state == "Composing":
                presence_data.update({
                    "details": "Composing",  
                    "small_image": "composing", 
                    "small_text": "Composing"
                })
                
            else:  
                presence_data.update({
                    "details": "Idle",  
                    "small_image": "idle",
                    "small_text": "Idle"
                })
            
            if plugin:
                presence_data["details"] += f" • {plugin}"
            
            
            # Update Discord
            self.rpc.update(**presence_data)
            print(f"🔄 Updated Discord: {presence_data['details']} @ {bpm} BPM" + (f" ({plugin})" if plugin else ""))
            
        except Exception as e:
            print(f"Failed to update Discord: {e}")
            self.connected = False
    
    def is_fl_studio_running(self):
        """Check if FL Studio process is actually running"""
        try:

            for proc in psutil.process_iter(['name']):
                if proc.info['name'] and 'fl' in proc.info['name'].lower():
                    return True
            return False
        except ImportError:
            try:
                file_age = time.time() - os.path.getmtime(self.state_file_path)
                return file_age < 10  
            except:
                return False
    
    def run(self):
        """Main monitoring loop"""
        if not self.connect_discord():
            print("Cannot connect to Discord, exiting...")
            return
            
        print("🔍 Monitoring FL Studio state... (Press Ctrl+C to stop)")
        
        try:
            fl_studio_was_running = False
            
            while True:
                # Check if FL Studio is actually running
                fl_running = self.is_fl_studio_running()
                
                if fl_running:
                    # FL Studio is running - read and update state
                    current_state = self.read_fl_state()
                    
                    if current_state:
                        # Check if state changed
                        state_key = f"{current_state['state']}|{current_state['bpm']}|{current_state['plugin']}"
                        last_key = f"{self.last_state.get('state', '')}|{self.last_state.get('bpm', 0)}|{self.last_state.get('plugin', '')}"
                        
                        if state_key != last_key:
                            self.update_discord_presence(current_state)
                            self.last_state = current_state
                    
                    fl_studio_was_running = True
                    
                else:
                    # FL Studio is not running
                    if fl_studio_was_running:
                        # FL Studio just closed - clear Discord presence
                        print("🔴 FL Studio closed - clearing Discord presence")
                        try:
                            self.rpc.clear()
                        except:
                            pass
                        self.last_state = {}
                        fl_studio_was_running = False
                    
                    # If no FL Studio, just wait
                    if not fl_studio_was_running:
                        print("⏳ Waiting for FL Studio to start...")
                        time.sleep(5)  # Check less frequently when FL Studio is closed
                        continue
                
                time.sleep(0.5)  # Check every 500ms when FL Studio is running
                
        except KeyboardInterrupt:
            print("\n👋 Shutting down...")
            
        finally:
            if self.rpc and self.connected:
                try:
                    self.rpc.close()
                    print("Disconnected from Discord")
                except:
                    pass

if __name__ == "__main__":
    client = FLStudioDiscordClient()
    
    # Don't require the state file to exist at startup
    print("Starting Discord client...")
    print("The client will wait for FL Studio to start and create the state file.")
    
    client.run()