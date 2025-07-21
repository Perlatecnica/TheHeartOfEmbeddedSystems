#!/usr/bin/env python3
"""
ESP8266 LED Controller - Simple Version
Minimal implementation without external dependencies
"""

import urllib.request
import urllib.error
import json
import sys


def send_command(ip_address, command, port=80):
    """
    Send a command to the ESP8266 web server

    Parameters:
        ip_address (str): IP address of the ESP8266 device
        command (str): Command to send ("ledon", "ledoff", "status")
        port (int): HTTP port (default is 80)
    """

    # Build the URL based on the command
    if command.lower() == "ledon":
        url = f"http://{ip_address}:{port}/ledon"
    elif command.lower() == "ledoff":
        url = f"http://{ip_address}:{port}/ledoff"
    elif command.lower() == "status":
        url = f"http://{ip_address}:{port}/status"
    else:
        print(f"Invalid command: {command}")
        print("Valid commands: ledon, ledoff, status")
        return

    try:
        # Send HTTP request to the server
        print(f"\nSending command: {command}")
        with urllib.request.urlopen(url, timeout=5) as response:
            # Read and decode the server response
            data = response.read().decode('utf-8')

            # Try to parse the response as JSON
            try:
                json_data = json.loads(data)
                print("\nResponse from server:")
                print(f"  Status: {json_data.get('status', 'unknown')}")
                if 'led' in json_data:
                    print(f"  LED: {json_data['led'].upper()}")
                if 'message' in json_data:
                    print(f"  Message: {json_data['message']}")
                if 'ip' in json_data:
                    print(f"  IP: {json_data['ip']}")
            except json.JSONDecodeError:
                print(f"Raw response: {data}")

    except urllib.error.URLError as e:
        print(f"Connection error: {e}")
        print("Please check if the IP is correct and the server is running.")
    except Exception as e:
        print(f"Unexpected error: {e}")


def main():
    """Main function"""

    # Check command-line arguments
    if len(sys.argv) < 2:
        print("Usage: python esp8266_led_controller.py <IP_ADDRESS>")
        print("Example: python esp8266_led_controller.py 192.168.1.123")
        sys.exit(1)

    ip_address = sys.argv[1]

    print(f"\nESP8266 LED Controller")
    print(f"Server: http://{ip_address}")
    print("\nAvailable commands: ledon, ledoff, status, quit")
    print("-" * 40)

    # Initial connection test
    send_command(ip_address, "status")

    # Command loop
    while True:
        try:
            # Ask the user for a command
            command = input("\n> ").strip().lower()

            if command in ["quit", "exit", "q"]:
                print("\nGoodbye!")
                break
            elif command in ["ledon", "ledoff", "status"]:
                send_command(ip_address, command)
            elif command == "":
                continue
            else:
                print("Invalid command. Use: ledon, ledoff, status, quit")

        except KeyboardInterrupt:
            print("\n\nProgram interrupted by user!")
            break
        except Exception as e:
            print(f"Unexpected error: {e}")


if __name__ == "__main__":
    main()
