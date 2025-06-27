// Copyright (C) 2019 Intel Corporation.  All rights reserved.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

const std = @import("std");
const c = @cImport({
    @cInclude("wasm_export.h");
    @cInclude("platform_common.h");
});
const program = @import("program.zig");

const ERROR_SIZE = 128;

const ProgramReturn = struct {
    return_value: u64,
    error_message: [ERROR_SIZE]u8,
};

// Function to read a file into a buffer and return the size
fn readFileAsBytes(allocator: std.mem.Allocator, path: []const u8) !struct { data: []u8, size: u32 } {
    const file = std.fs.cwd().openFile(path, .{}) catch |err| {
        std.debug.print("Failed to open file: {s}\n", .{path});
        return err;
    };
    defer file.close();

    const file_size = try file.getEndPos();
    if (file_size > std.math.maxInt(u32)) {
        std.debug.print("File too large\n", .{});
        return error.FileTooLarge;
    }

    const buffer = try allocator.alloc(u8, file_size);
    const read_size = try file.readAll(buffer);

    if (read_size != file_size) {
        std.debug.print("Failed to read entire file\n", .{});
        allocator.free(buffer);
        return error.IncompleteRead;
    }

    return .{ .data = buffer, .size = @intCast(file_size) };
}

fn myLog(log_level: c.uint32, file: [*c]const u8, line: c_int, fmt: [*c]const u8, args: anytype) void {
    _ = file;
    _ = line;
    _ = args;

    var buf: [200]u8 = undefined;

    // Add newline to fmt if not present
    const fmt_len = std.mem.len(fmt);
    const needs_newline = fmt_len == 0 or fmt[fmt_len - 1] != '\n';

    const log_prefix = if (log_level == c.WASM_LOG_LEVEL_VERBOSE) "[WamrLogger - VERBOSE] " else "[WamrLogger] ";

    if (needs_newline) {
        _ = std.fmt.bufPrint(&buf, "{s}{s}\n", .{ log_prefix, fmt }) catch return;
    } else {
        _ = std.fmt.bufPrint(&buf, "{s}{s}", .{ log_prefix, fmt }) catch return;
    }

    std.debug.print("{s}", .{buf});
}

fn myVprintf(format: [*c]const u8, ap: std.builtin.VaList) c_int {
    // Print in blue
    var buf: [200]u8 = undefined;
    _ = std.fmt.bufPrint(&buf, "\x1b[34m{s}\x1b[0m", .{format}) catch return -1;
    return c.vprintf(&buf, ap);
}

fn printUsage() void {
    std.debug.print("Options:\r\n", .{});
    std.debug.print("  -f [path of wasm file] \n", .{});
    std.debug.print("  -i [number of iterations] (default: 10000) \n", .{});
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    var wasm_path: ?[]const u8 = null;
    var iterations: i32 = 10000;

    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        if (std.mem.eql(u8, args[i], "-f") and i + 1 < args.len) {
            i += 1;
            wasm_path = args[i];
        } else if (std.mem.eql(u8, args[i], "-i") and i + 1 < args.len) {
            i += 1;
            iterations = std.fmt.parseInt(i32, args[i], 10) catch {
                std.debug.print("Invalid iterations value\n", .{});
                return;
            };
        } else if (std.mem.eql(u8, args[i], "-h")) {
            printUsage();
            return;
        } else {
            printUsage();
            return;
        }
    }

    if (wasm_path == null) {
        printUsage();
        return;
    }

    const file_data = readFileAsBytes(allocator, wasm_path.?) catch |err| {
        std.debug.print("Failed to read file: {}\n", .{err});
        return;
    };
    defer allocator.free(file_data.data);

    const package_type = c.get_package_type(file_data.data.ptr, file_data.size);
    std.debug.print("Package type for file of size {d}: {d}\n", .{ file_data.size, package_type });

    var new_heap: [512 * 1024]u8 = undefined;

    const program_result = program.runProgram(file_data.data.ptr, file_data.size, &new_heap, new_heap.len, iterations);

    std.debug.print("Program return value: {d}\n", .{program_result.return_value});
    std.debug.print("Program error message: {s}\n", .{program_result.error_message});
}

