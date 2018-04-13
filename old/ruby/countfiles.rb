#!/usr/bin/ruby --disable=gems

require "rbconfig"
require "ostruct"
require "optparse"
require_relative File.join(ENV["HOME"], "/dev/gems/srcwalk/lib/srcwalk")
require_relative File.join(ENV["HOME"], "/dev/gems/spinner/lib/spinner")
require_relative File.join(ENV["HOME"], "/dev/gems/rbfind/lib/rbfind")

module Util
  def self.windows?
    return (RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/)
  end

  def self.use_icase?
    return windows?
  end

  def self.extname(basename)
    ext = File.extname(basename)
    if ext.empty? then
      return basename
    end
    return ext
  end
end

class FileCounter
  def initialize(opts)
    @opts = opts
    @map = Hash.new(0)
  end

  def push(str, fbase)
    str = (if fbase.start_with?(".") then fbase else Util::extname(fbase) end)
    str.downcase! if @opts[:icase]
    @map[str] += 1
  end

  def padsize(min: 13)
    val = @map.keys.max_by(&:length)
    val = (val ? val.length : 0)
    if val < min then
      return min
    end
    return val
  end

  def each(&block)
   @map.sort_by{|ext, count| count}.each(&block)
  end
end

def stats(counter, directory)
  puts("file statistics for #{directory}:")
  pad = counter.padsize
  counter.each do |ext, count|
    printf("  %-#{pad + 2}s %d\n", ext, count)
  end
end

def test(item, opts)
  if opts[:ign_noext] then
    if File.basename(item).count(".") == 0 then
      return nil
    end
  end
  return item
end

def countfiles_phys(directory, opts)
  counter = FileCounter.new(opts)
  $stderr.print("[counttypes] collecting files ... ") if not opts[:verbose]
  SourceWalk::walk(directory, verbose: opts[:verbose]) do |path, finished, i|
    #Spinner::spin(sleep: nil) if not opts[:verbose]
    fbase = File.basename(path)
    if (item = test(fbase, opts)) != nil then
      counter.push(path, fbase)
    end
  end
  Spinner::clear
  stats(counter, directory)
end

def countfiles_stdin(dirname, opts)
  counter = FileCounter.new(opts)
  $stderr.puts("[counttypes] reading paths from stdin ...")
  $stdin.each_line do |line|
    line.strip!
    fbase = File.basename(line)
    if (item = test(fbase, opts)) != nil then
      if File.exist?(line) && File.directory?(line) then
        next
      end
      counter.push(line, fbase)
    end
  end
  stats(counter, dirname)
end

begin
  $stdout.sync = true
  selfname = File.basename($0)
  opts = {
    ignore: [],
    ign_noext: false,
    verbose: false,
    readstdin: false,
    icase: false,
  }
  OptionParser.new {|prs|
    prs.banner = "Usage: #{selfname} [options] [<directories ...>]"
    prs.on("-i<dir>", "--ignore=<dir>", "Add <dir> to list of dirs to be ignored"){|v|
      opts[:ignore].push(v)
    }
    prs.on("-v", "--[no-]verbose", "Toggle verbosity"){|v|
      opts[:verbose] = v
    }
    prs.on(nil, "--stdin", "Read paths from stdin (i.e., via 'find')"){|v|
      opts[:readstdin] = true
    }
    prs.on("-c", "--[no-]case-insensitive", "ignore case-sensitivity (mostly only useful on windows)"){|v|
      opts[:icase] = true
    }
    prs.on("-n", "--ignore-noext", "ignore files without an extension"){|v|
      opts[:ign_noext] = true
    }
  }.parse!
  if not opts[:ignore].empty? then
    opts[:ignore].each do |d|
      SourceWalk.add_badpath(d)
    end
  end
  if (opts[:readstdin]) || (ARGV.empty? && (not $stdin.tty?)) then
    countfiles_stdin("(stdin)", opts)
  else
    ARGV.push(".") if ARGV.empty?
    ARGV.each do |d|
      countfiles_phys(d, opts)
    end
  end
end

