#!/usr/bin/env python3
import requests
import sys
import os
from pathlib import Path

class LANDriveClient:
    def __init__(self, server_url="http://192.168.1.180:8080"):
        self.server_url = server_url.rstrip('/')
        self.headers = {
            "Content-Type": "application/octet-stream"
        }
    
    def upload_file(self, file_path):
        """Upload a file to the server"""
        if not os.path.exists(file_path):
            print(f"Error: File '{file_path}' not found")
            return False
        
        filename = os.path.basename(file_path)
        upload_headers = self.headers.copy()
        upload_headers["X-Filename"] = filename
        
        try:
            with open(file_path, 'rb') as f:
                print(f"Uploading {filename}...")
                response = requests.post(
                    f"{self.server_url}/api/upload",
                    headers=upload_headers,
                    data=f
                )
            
            if response.status_code == 200:
                print(f"✓ Upload successful: {filename}")
                return True
            else:
                print(f"✗ Upload failed: {response.text}")
                return False
                
        except Exception as e:
            print(f"✗ Upload error: {e}")
            return False
    
    def download_file(self, filename, save_path=None):
        """Download a file from the server"""
        if save_path is None:
            save_path = filename
        
        try:
            print(f"Downloading {filename}...")
            response = requests.get(
                f"{self.server_url}/api/download/{filename}",
                stream=True
            )
            
            if response.status_code == 200:
                with open(save_path, 'wb') as f:
                    for chunk in response.iter_content(chunk_size=8192):
                        if chunk:
                            f.write(chunk)
                print(f"✓ Download successful: {save_path}")
                return True
            else:
                print(f"✗ Download failed: {response.text}")
                return False
                
        except Exception as e:
            print(f"✗ Download error: {e}")
            return False
    
    def list_files(self):
        """List all files on the server"""
        try:
            response = requests.get(
                f"{self.server_url}/api/files")
            
            if response.status_code == 200:
                data = response.json()
                files = data.get("files", [])
                
                if not files:
                    print("No files on server")
                    return []
                
                print(f"\nFiles on server ({len(files)} total):")
                print("-" * 60)
                print(f"{'Name':<30} {'Size':<12} {'Modified':<18}")
                print("-" * 60)
                
                for file_info in files:
                    name = file_info.get("name", "Unknown")
                    size = self.format_size(file_info.get("size", 0))
                    modified = self.format_timestamp(file_info.get("modified", 0))
                    print(f"{name:<30} {size:<12} {modified:<18}")
                
                return files
            else:
                print(f"✗ Failed to list files: {response.text}")
                return []
                
        except Exception as e:
            print(f"✗ List files error: {e}")
            return []
    
    def delete_file(self, filename):
        """Delete a file from the server"""
        try:
            response = requests.delete(
                f"{self.server_url}/api/files/{filename}")
            
            if response.status_code == 200:
                print(f"✓ File deleted: {filename}")
                return True
            else:
                print(f"✗ Delete failed: {response.text}")
                return False
                
        except Exception as e:
            print(f"✗ Delete error: {e}")
            return False
    
    def get_file_info(self, filename):
        """Get information about a specific file"""
        try:
            response = requests.get(
                f"{self.server_url}/api/info/{filename}" )
            
            if response.status_code == 200:
                info = response.json()
                print(f"\nFile Info: {filename}")
                print("-" * 30)
                print(f"Name: {info.get('name', 'Unknown')}")
                print(f"Size: {self.format_size(info.get('size', 0))}")
                print(f"Modified: {self.format_timestamp(info.get('modified', 0))}")
                return info
            else:
                print(f"✗ Failed to get file info: {response.text}")
                return None
                
        except Exception as e:
            print(f"✗ File info error: {e}")
            return None
    
    @staticmethod
    def format_size(size_bytes):
        """Format file size in human readable format"""
        if size_bytes == 0:
            return "0 B"
        
        size_names = ["B", "KB", "MB", "GB"]
        i = 0
        while size_bytes >= 1024 and i < len(size_names) - 1:
            size_bytes /= 1024.0
            i += 1
        
        return f"{size_bytes:.1f} {size_names[i]}"
    
    @staticmethod
    def format_timestamp(timestamp):
        """Format timestamp to readable date"""
        if timestamp == 0:
            return "Unknown"
        
        import datetime
        return datetime.datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M")

def print_usage():
    """Print usage instructions"""
    print("LAN Drive Client")
    print("Usage:")
    print("  python client.py upload <file_path>     # Upload a file")
    print("  python client.py download <filename>    # Download a file")
    print("  python client.py list                   # List all files")
    print("  python client.py delete <filename>      # Delete a file")
    print("  python client.py info <filename>        # Get file information")
    print("\nExamples:")
    print("  python client.py upload document.pdf")
    print("  python client.py download document.pdf")
    print("  python client.py list")

def main():
    if len(sys.argv) < 2:
        print_usage()
        return
    
    client = LANDriveClient("http://192.168.1.180:8080")
    
    command = sys.argv[1].lower()
    
    if command == "upload":
        if len(sys.argv) != 3:
            print("Usage: python client.py upload <file_path>")
            return
        file_path = sys.argv[2]
        client.upload_file(file_path)
    
    elif command == "download":
        if len(sys.argv) != 3:
            print("Usage: python client.py download <filename>")
            return
        filename = sys.argv[2]
        client.download_file(filename)
    
    elif command == "list":
        client.list_files()
    
    elif command == "delete":
        if len(sys.argv) != 3:
            print("Usage: python client.py delete <filename>")
            return
        filename = sys.argv[2]
        
        # Confirmation prompt
        response = input(f"Are you sure you want to delete '{filename}'? (y/N): ")
        if response.lower() in ['y', 'yes']:
            client.delete_file(filename)
        else:
            print("Delete cancelled")
    
    elif command == "info":
        if len(sys.argv) != 3:
            print("Usage: python client.py info <filename>")
            return
        filename = sys.argv[2]
        client.get_file_info(filename)
    
    else:
        print(f"Unknown command: {command}")
        print_usage()

if __name__ == "__main__":
    main()
