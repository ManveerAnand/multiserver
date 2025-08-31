#!/usr/bin/env python3
"""
MultiServer Chat Benchmarking Tool
Provides sophisticated chat server performance testing
"""

import socket
import threading
import time
import argparse
import sys
import json
from datetime import datetime
import queue
import statistics


class ChatBenchmark:
    def __init__(self, host='localhost', port=8081):
        self.host = host
        self.port = port
        self.results = {
            'total_connections': 0,
            'successful_connections': 0,
            'failed_connections': 0,
            'total_messages': 0,
            'successful_messages': 0,
            'failed_messages': 0,
            'response_times': [],
            'connection_times': [],
            'errors': [],
            'start_time': None,
            'end_time': None
        }
        self.result_queue = queue.Queue()

    def connect_and_test(self, test_id, messages_per_connection=10, delay=0.1):
        """Single connection test thread"""
        connection_start = time.time()

        try:
            # Create socket connection
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5.0)  # 5 second timeout

            # Connect to server
            sock.connect((self.host, self.port))
            connection_time = time.time() - connection_start

            self.result_queue.put(('connection_success', connection_time))

            # Send test messages
            for i in range(messages_per_connection):
                message_start = time.time()

                # Send different types of commands
                commands = [
                    "HELP",
                    "TIME",
                    "STATUS",
                    f"ECHO Test message {test_id}-{i}",
                ]

                command = commands[i % len(commands)]

                try:
                    # Send command
                    sock.send(f"{command}\n".encode())

                    # Receive response
                    response = sock.recv(1024).decode()

                    response_time = time.time() - message_start

                    if response:
                        self.result_queue.put(
                            ('message_success', response_time))
                    else:
                        self.result_queue.put(('message_fail', 'No response'))

                except Exception as e:
                    self.result_queue.put(('message_fail', str(e)))

                # Delay between messages
                if delay > 0:
                    time.sleep(delay)

            # Clean disconnect
            sock.send(b"QUIT\n")
            sock.close()

        except Exception as e:
            self.result_queue.put(('connection_fail', str(e)))

    def run_concurrent_test(self, num_connections=100, messages_per_connection=10, delay=0.1):
        """Run concurrent connection test"""
        print(
            f"🚀 Starting concurrent test: {num_connections} connections, {messages_per_connection} messages each")

        self.results['start_time'] = datetime.now()
        start_time = time.time()

        # Start all threads
        threads = []
        for i in range(num_connections):
            thread = threading.Thread(
                target=self.connect_and_test,
                args=(i, messages_per_connection, delay)
            )
            threads.append(thread)
            thread.start()

            # Small delay to prevent overwhelming the server
            time.sleep(0.01)

        # Wait for all threads to complete
        for thread in threads:
            thread.join()

        end_time = time.time()
        self.results['end_time'] = datetime.now()

        # Process results from queue
        while not self.result_queue.empty():
            result_type, data = self.result_queue.get()

            if result_type == 'connection_success':
                self.results['successful_connections'] += 1
                self.results['connection_times'].append(data)
            elif result_type == 'connection_fail':
                self.results['failed_connections'] += 1
                self.results['errors'].append(f"Connection error: {data}")
            elif result_type == 'message_success':
                self.results['successful_messages'] += 1
                self.results['response_times'].append(data)
            elif result_type == 'message_fail':
                self.results['failed_messages'] += 1
                self.results['errors'].append(f"Message error: {data}")

        self.results['total_connections'] = num_connections
        self.results['total_messages'] = num_connections * \
            messages_per_connection
        self.results['duration'] = end_time - start_time

        return self.results

    def run_sustained_load_test(self, duration_seconds=60, connections_per_second=10):
        """Run sustained load test"""
        print(
            f"🔥 Starting sustained load test: {duration_seconds}s duration, {connections_per_second} conn/sec")

        self.results['start_time'] = datetime.now()
        start_time = time.time()
        end_time = start_time + duration_seconds

        connection_count = 0

        while time.time() < end_time:
            # Start batch of connections
            threads = []
            for i in range(connections_per_second):
                thread = threading.Thread(
                    target=self.connect_and_test,
                    args=(connection_count + i, 1, 0)  # 1 message, no delay
                )
                threads.append(thread)
                thread.start()
                connection_count += 1

            # Wait for this batch to complete
            for thread in threads:
                thread.join()

            # Brief pause before next batch
            time.sleep(0.5)

        self.results['end_time'] = datetime.now()
        self.results['duration'] = time.time() - start_time
        self.results['total_connections'] = connection_count

        # Process results
        while not self.result_queue.empty():
            result_type, data = self.result_queue.get()

            if result_type == 'connection_success':
                self.results['successful_connections'] += 1
                self.results['connection_times'].append(data)
            elif result_type == 'connection_fail':
                self.results['failed_connections'] += 1
                self.results['errors'].append(f"Connection error: {data}")
            elif result_type == 'message_success':
                self.results['successful_messages'] += 1
                self.results['response_times'].append(data)
            elif result_type == 'message_fail':
                self.results['failed_messages'] += 1
                self.results['errors'].append(f"Message error: {data}")

        return self.results

    def print_results(self):
        """Print comprehensive test results"""
        results = self.results

        print("\n" + "="*60)
        print("📊 CHAT BENCHMARK RESULTS")
        print("="*60)

        print(f"⏱️  Duration: {results['duration']:.2f} seconds")
        print(f"📅 Start: {results['start_time']}")
        print(f"📅 End: {results['end_time']}")

        print(f"\n🔗 CONNECTION STATISTICS:")
        print(f"   Total Attempted: {results['total_connections']}")
        print(f"   Successful: {results['successful_connections']}")
        print(f"   Failed: {results['failed_connections']}")

        if results['connection_times']:
            conn_times = results['connection_times']
            print(
                f"   Avg Connection Time: {statistics.mean(conn_times):.3f}s")
            print(f"   Min Connection Time: {min(conn_times):.3f}s")
            print(f"   Max Connection Time: {max(conn_times):.3f}s")

        print(f"\n💬 MESSAGE STATISTICS:")
        print(f"   Total Messages: {results['total_messages']}")
        print(f"   Successful: {results['successful_messages']}")
        print(f"   Failed: {results['failed_messages']}")

        if results['response_times']:
            resp_times = results['response_times']
            print(f"   Avg Response Time: {statistics.mean(resp_times):.3f}s")
            print(f"   Min Response Time: {min(resp_times):.3f}s")
            print(f"   Max Response Time: {max(resp_times):.3f}s")

            # Percentiles
            sorted_times = sorted(resp_times)
            p50 = sorted_times[len(sorted_times)//2]
            p95 = sorted_times[int(len(sorted_times)*0.95)]
            p99 = sorted_times[int(len(sorted_times)*0.99)]

            print(f"   50th Percentile: {p50:.3f}s")
            print(f"   95th Percentile: {p95:.3f}s")
            print(f"   99th Percentile: {p99:.3f}s")

        print(f"\n📈 PERFORMANCE METRICS:")
        success_rate = (results['successful_connections'] / results['total_connections']
                        * 100) if results['total_connections'] > 0 else 0
        print(f"   Connection Success Rate: {success_rate:.1f}%")

        msg_success_rate = (results['successful_messages'] / results['total_messages']
                            * 100) if results['total_messages'] > 0 else 0
        print(f"   Message Success Rate: {msg_success_rate:.1f}%")

        conn_per_sec = results['successful_connections'] / \
            results['duration'] if results['duration'] > 0 else 0
        print(f"   Connections/Second: {conn_per_sec:.1f}")

        msg_per_sec = results['successful_messages'] / \
            results['duration'] if results['duration'] > 0 else 0
        print(f"   Messages/Second: {msg_per_sec:.1f}")

        if results['errors']:
            print(f"\n❌ ERRORS ({len(results['errors'])}):")
            error_summary = {}
            for error in results['errors'][:10]:  # Show first 10
                error_summary[error] = error_summary.get(error, 0) + 1

            for error, count in error_summary.items():
                print(f"   {error} (x{count})")

            if len(results['errors']) > 10:
                print(f"   ... and {len(results['errors']) - 10} more errors")

        print("="*60)

    def save_results_json(self, filename):
        """Save results to JSON file"""
        # Convert datetime objects to strings for JSON serialization
        json_results = self.results.copy()
        if json_results['start_time']:
            json_results['start_time'] = json_results['start_time'].isoformat()
        if json_results['end_time']:
            json_results['end_time'] = json_results['end_time'].isoformat()

        with open(filename, 'w') as f:
            json.dump(json_results, f, indent=2)

        print(f"📁 Results saved to: {filename}")


def main():
    parser = argparse.ArgumentParser(
        description="MultiServer Chat Benchmark Tool")
    parser.add_argument("--host", default="localhost", help="Server host")
    parser.add_argument("--port", type=int, default=8081, help="Server port")
    parser.add_argument("--connections", type=int, default=100,
                        help="Number of concurrent connections")
    parser.add_argument("--messages", type=int, default=10,
                        help="Messages per connection")
    parser.add_argument("--delay", type=float, default=0.1,
                        help="Delay between messages")
    parser.add_argument(
        "--test-type", choices=["concurrent", "sustained"], default="concurrent", help="Test type")
    parser.add_argument("--duration", type=int, default=60,
                        help="Duration for sustained test (seconds)")
    parser.add_argument("--rate", type=int, default=10,
                        help="Connections per second for sustained test")
    parser.add_argument("--output", help="Save results to JSON file")

    args = parser.parse_args()

    # Check if server is reachable
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((args.host, args.port))
        sock.close()
        print(f"✅ Chat server reachable at {args.host}:{args.port}")
    except Exception as e:
        print(
            f"❌ Cannot connect to chat server at {args.host}:{args.port}: {e}")
        sys.exit(1)

    # Run benchmark
    benchmark = ChatBenchmark(args.host, args.port)

    if args.test_type == "concurrent":
        benchmark.run_concurrent_test(
            args.connections, args.messages, args.delay)
    else:  # sustained
        benchmark.run_sustained_load_test(args.duration, args.rate)

    # Display results
    benchmark.print_results()

    # Save to file if requested
    if args.output:
        benchmark.save_results_json(args.output)


if __name__ == "__main__":
    main()
