#!/bin/bash

# MultiServer Performance Benchmarking Suite
# This script runs comprehensive performance tests on the MultiServer

echo "==================================="
echo "  MultiServer Performance Benchmark"
echo "==================================="

SERVER_HOST="localhost"
HTTP_PORT="8080"
CHAT_PORT="8081"
RESULTS_DIR="./benchmark_results"

# Create results directory
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Check if server is running
check_server() {
    echo "Checking if MultiServer is running..."
    if ! nc -z $SERVER_HOST $HTTP_PORT; then
        echo "❌ HTTP server not running on port $HTTP_PORT"
        echo "Please start MultiServer first: ./multiserver -c config/multiserver.conf"
        exit 1
    fi
    
    if ! nc -z $SERVER_HOST $CHAT_PORT; then
        echo "❌ Chat server not running on port $CHAT_PORT"
        exit 1
    fi
    
    echo "✅ MultiServer is running"
}

# HTTP Performance Tests
http_benchmark() {
    echo ""
    echo "🌐 HTTP Server Benchmarking"
    echo "================================"
    
    local results_file="$RESULTS_DIR/http_benchmark_$TIMESTAMP.txt"
    
    echo "Test 1: Basic Load Test (1000 requests, 10 concurrent)" | tee -a "$results_file"
    ab -n 1000 -c 10 "http://$SERVER_HOST:$HTTP_PORT/" 2>&1 | tee -a "$results_file"
    
    echo "" | tee -a "$results_file"
    echo "Test 2: Medium Load Test (5000 requests, 50 concurrent)" | tee -a "$results_file"
    ab -n 5000 -c 50 "http://$SERVER_HOST:$HTTP_PORT/" 2>&1 | tee -a "$results_file"
    
    echo "" | tee -a "$results_file"
    echo "Test 3: High Load Test (10000 requests, 100 concurrent)" | tee -a "$results_file"
    ab -n 10000 -c 100 "http://$SERVER_HOST:$HTTP_PORT/" 2>&1 | tee -a "$results_file"
    
    echo "" | tee -a "$results_file"
    echo "Test 4: Keep-Alive Test (5000 requests, 50 concurrent, keep-alive)" | tee -a "$results_file"
    ab -n 5000 -c 50 -k "http://$SERVER_HOST:$HTTP_PORT/" 2>&1 | tee -a "$results_file"
    
    echo "HTTP benchmark results saved to: $results_file"
}

# Chat Performance Tests
chat_benchmark() {
    echo ""
    echo "💬 Chat Server Benchmarking"
    echo "================================"
    
    local results_file="$RESULTS_DIR/chat_benchmark_$TIMESTAMP.txt"
    
    echo "Test 1: Connection Test (100 sequential connections)" | tee -a "$results_file"
    start_time=$(date +%s.%N)
    for i in {1..100}; do
        echo "HELP" | nc -w 1 $SERVER_HOST $CHAT_PORT > /dev/null 2>&1
    done
    end_time=$(date +%s.%N)
    duration=$(echo "$end_time - $start_time" | bc)
    echo "Sequential connections completed in: ${duration}s" | tee -a "$results_file"
    
    echo "" | tee -a "$results_file"
    echo "Test 2: Concurrent Connection Test (50 parallel connections)" | tee -a "$results_file"
    start_time=$(date +%s.%N)
    for i in {1..50}; do
        (echo "STATUS" | nc -w 2 $SERVER_HOST $CHAT_PORT > /dev/null 2>&1) &
    done
    wait
    end_time=$(date +%s.%N)
    duration=$(echo "$end_time - $start_time" | bc)
    echo "Parallel connections completed in: ${duration}s" | tee -a "$results_file"
    
    echo "" | tee -a "$results_file"
    echo "Test 3: Message Throughput Test" | tee -a "$results_file"
    start_time=$(date +%s.%N)
    for i in {1..200}; do
        echo "ECHO Test message $i" | nc -w 1 $SERVER_HOST $CHAT_PORT > /dev/null 2>&1
    done
    end_time=$(date +%s.%N)
    duration=$(echo "$end_time - $start_time" | bc)
    messages_per_sec=$(echo "scale=2; 200 / $duration" | bc)
    echo "Message throughput: ${messages_per_sec} messages/sec" | tee -a "$results_file"
    
    echo "Chat benchmark results saved to: $results_file"
}

# System Resource Monitoring
resource_monitor() {
    echo ""
    echo "📊 System Resource Monitoring"
    echo "================================"
    
    local results_file="$RESULTS_DIR/resources_$TIMESTAMP.txt"
    
    echo "System Information:" | tee -a "$results_file"
    echo "CPU: $(grep 'model name' /proc/cpuinfo | head -1 | cut -d':' -f2 | xargs)" | tee -a "$results_file"
    echo "Memory: $(free -h | grep '^Mem:' | awk '{print $2}')" | tee -a "$results_file"
    echo "OS: $(uname -a)" | tee -a "$results_file"
    echo "" | tee -a "$results_file"
    
    # Get MultiServer process info
    local pid=$(pgrep multiserver)
    if [ ! -z "$pid" ]; then
        echo "MultiServer Process Info (PID: $pid):" | tee -a "$results_file"
        echo "Memory Usage: $(ps -p $pid -o rss= | awk '{print $1/1024 " MB"}')" | tee -a "$results_file"
        echo "CPU Usage: $(ps -p $pid -o %cpu= | xargs)%" | tee -a "$results_file"
        echo "Open Files: $(ls /proc/$pid/fd | wc -l)" | tee -a "$results_file"
        echo "Threads: $(ps -p $pid -o nlwp= | xargs)" | tee -a "$results_file"
    else
        echo "MultiServer process not found" | tee -a "$results_file"
    fi
    
    echo "Resource monitoring saved to: $results_file"
}

# Connection Scaling Test
scaling_test() {
    echo ""
    echo "📈 Connection Scaling Test"
    echo "================================"
    
    local results_file="$RESULTS_DIR/scaling_$TIMESTAMP.txt"
    
    echo "Testing connection scaling..." | tee -a "$results_file"
    
    for connections in 10 25 50 100 250 500; do
        echo "" | tee -a "$results_file"
        echo "Testing with $connections concurrent connections:" | tee -a "$results_file"
        
        start_time=$(date +%s.%N)
        
        # Start concurrent connections
        for ((i=1; i<=connections; i++)); do
            (echo "STATUS" | nc -w 3 $SERVER_HOST $CHAT_PORT > /dev/null 2>&1) &
        done
        
        # Wait for all to complete
        wait
        
        end_time=$(date +%s.%N)
        duration=$(echo "$end_time - $start_time" | bc)
        
        echo "Handled $connections connections in ${duration}s" | tee -a "$results_file"
        
        # Brief pause between tests
        sleep 2
    done
    
    echo "Scaling test results saved to: $results_file"
}

# Memory Leak Detection
memory_test() {
    echo ""
    echo "🧠 Memory Leak Detection"
    echo "================================"
    
    local results_file="$RESULTS_DIR/memory_$TIMESTAMP.txt"
    local pid=$(pgrep multiserver)
    
    if [ -z "$pid" ]; then
        echo "MultiServer process not found for memory testing" | tee -a "$results_file"
        return
    fi
    
    echo "Monitoring memory usage during load test..." | tee -a "$results_file"
    
    # Record initial memory usage
    initial_mem=$(ps -p $pid -o rss= | xargs)
    echo "Initial memory: ${initial_mem} KB" | tee -a "$results_file"
    
    # Run load test while monitoring memory
    echo "Running sustained load test..." | tee -a "$results_file"
    
    # Background load generation
    for i in {1..5}; do
        (
            for j in {1..100}; do
                curl -s "http://$SERVER_HOST:$HTTP_PORT/" > /dev/null 2>&1
                echo "HELP" | nc -w 1 $SERVER_HOST $CHAT_PORT > /dev/null 2>&1
            done
        ) &
    done
    
    # Monitor memory every 5 seconds for 1 minute
    for i in {1..12}; do
        sleep 5
        current_mem=$(ps -p $pid -o rss= | xargs)
        echo "Memory at ${i}0s: ${current_mem} KB" | tee -a "$results_file"
    done
    
    # Wait for background jobs to finish
    wait
    
    # Final memory reading
    sleep 5
    final_mem=$(ps -p $pid -o rss= | xargs)
    echo "Final memory: ${final_mem} KB" | tee -a "$results_file"
    
    # Calculate difference
    mem_diff=$((final_mem - initial_mem))
    echo "Memory difference: ${mem_diff} KB" | tee -a "$results_file"
    
    if [ $mem_diff -gt 1000 ]; then
        echo "⚠️  Potential memory leak detected!" | tee -a "$results_file"
    else
        echo "✅ No significant memory leak detected" | tee -a "$results_file"
    fi
    
    echo "Memory test results saved to: $results_file"
}

# Generate Summary Report
generate_report() {
    echo ""
    echo "📋 Generating Summary Report"
    echo "================================"
    
    local report_file="$RESULTS_DIR/summary_report_$TIMESTAMP.html"
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>MultiServer Performance Report - $TIMESTAMP</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { background: #667eea; color: white; padding: 20px; border-radius: 10px; }
        .section { margin: 20px 0; padding: 15px; border-left: 4px solid #667eea; }
        .metric { background: #f0f0f0; padding: 10px; margin: 5px 0; border-radius: 5px; }
        .good { color: green; font-weight: bold; }
        .warning { color: orange; font-weight: bold; }
        .error { color: red; font-weight: bold; }
    </style>
</head>
<body>
    <div class="header">
        <h1>🚀 MultiServer Performance Report</h1>
        <p>Generated: $(date)</p>
    </div>
    
    <div class="section">
        <h2>📊 Test Summary</h2>
        <div class="metric">HTTP Benchmarks: ✅ Completed</div>
        <div class="metric">Chat Benchmarks: ✅ Completed</div>
        <div class="metric">Resource Monitoring: ✅ Completed</div>
        <div class="metric">Scaling Tests: ✅ Completed</div>
        <div class="metric">Memory Tests: ✅ Completed</div>
    </div>
    
    <div class="section">
        <h2>📁 Detailed Results</h2>
        <p>All detailed results are saved in the benchmark_results directory:</p>
        <ul>
            <li>HTTP benchmarks: http_benchmark_$TIMESTAMP.txt</li>
            <li>Chat benchmarks: chat_benchmark_$TIMESTAMP.txt</li>
            <li>Resource monitoring: resources_$TIMESTAMP.txt</li>
            <li>Scaling tests: scaling_$TIMESTAMP.txt</li>
            <li>Memory tests: memory_$TIMESTAMP.txt</li>
        </ul>
    </div>
    
    <div class="section">
        <h2>🎯 Next Steps</h2>
        <p>Based on the results, consider:</p>
        <ul>
            <li>Optimizing bottlenecks identified in the tests</li>
            <li>Implementing connection pooling improvements</li>
            <li>Adding caching for frequently requested resources</li>
            <li>Implementing rate limiting for security</li>
        </ul>
    </div>
</body>
</html>
EOF

    echo "Summary report generated: $report_file"
    echo "Open it in a browser to view the formatted results."
}

# Main execution
main() {
    echo "Starting MultiServer performance benchmark suite..."
    
    # Check dependencies
    command -v ab >/dev/null 2>&1 || { echo "❌ Apache Bench (ab) not found. Install with: sudo apt install apache2-utils"; exit 1; }
    command -v nc >/dev/null 2>&1 || { echo "❌ netcat (nc) not found. Install with: sudo apt install netcat"; exit 1; }
    command -v bc >/dev/null 2>&1 || { echo "❌ bc calculator not found. Install with: sudo apt install bc"; exit 1; }
    
    check_server
    
    # Run all benchmarks
    http_benchmark
    chat_benchmark
    resource_monitor
    scaling_test
    memory_test
    generate_report
    
    echo ""
    echo "🎉 Benchmark suite completed!"
    echo "Results saved in: $RESULTS_DIR"
    echo ""
    echo "Quick summary:"
    echo "- HTTP tests: Check requests/second and response times"
    echo "- Chat tests: Check message throughput and connection handling"  
    echo "- Scaling: Check how performance changes with load"
    echo "- Memory: Check for memory leaks during sustained load"
}

# Run the benchmark suite
main "$@"
