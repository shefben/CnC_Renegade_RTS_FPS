using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;

namespace makemix
{
    class Program
    {
        static void ShowUsage()
        {
            Console.WriteLine();
            Console.WriteLine("makemix is a tool to package all the files inside a directory into a Renegade MIX file\n");
            Console.WriteLine("usage: makemix <directory> [-o <mix file>] [-i <include>...] [-e <exclude>...]\n");
        }

        static int Main(string[] args)
        {
            if (args.Length == 0)
            {
                ShowUsage();
                return 1;
            }

            string input_directory = null;
            string output_path = null;

            var include_patterns = new HashSet<string>();
            var exclude_patterns = new HashSet<string>();

            try
            {
                input_directory = Path.GetFullPath(args[0].TrimEnd('/', '\\'));

                int mode = 0;
                for (int i = 1; i < args.Length; ++i)
                {
                    var arg = args[i].Trim();
                    switch (arg)
                    {
                        case "-o":
                            mode = 1;
                            break;
                        case "-i":
                            mode = 2;
                            break;
                        case "-e":
                            mode = 3;
                            break;
                        default:
                            switch (mode)
                            {
                                case 1:
                                    if (output_path != null) throw new ArgumentException();
                                    output_path = Path.GetFullPath(arg);
                                    break;
                                case 2:
                                    include_patterns.Add(arg);
                                    break;
                                case 3:
                                    exclude_patterns.Add(arg);
                                    break;
                                default: throw new ArgumentException();
                            }
                            break;
                    }
                }

                if (!Directory.Exists(input_directory)) throw new ArgumentException();

            }
            catch (ArgumentException)
            {
                ShowUsage();
                return 1;
            }

            if (output_path == null) output_path = Path.ChangeExtension(input_directory, ".mix");

            try
            {
                var current_directory = Environment.CurrentDirectory;
                Environment.CurrentDirectory = input_directory;
                var files = BuildFileList(include_patterns, exclude_patterns);
                Environment.CurrentDirectory = current_directory;

                BuildMixFile(output_path, files);

                return 0;
            }
            catch (System.Exception ex)
            {
                Console.Error.WriteLine();
                Console.Error.Write(ex.ToString());
                Console.Error.WriteLine();

                return 1;
            }
        }

        static HashSet<string> BuildFileList(IEnumerable<string> include_patterns, IEnumerable<string> exclude_patterns)
        {
            var files = new HashSet<string>();

            if (include_patterns.Any())
            {
                foreach (string include_pattern in include_patterns)
                {
                    files.UnionWith(EnumeratePattern(include_pattern));
                }
            }
            else
            {
                files.UnionWith(EnumeratePattern("*"));
            }

            if (exclude_patterns.Any())
            {
                foreach (string exclude_pattern in exclude_patterns)
                {
                    files.ExceptWith(EnumeratePattern(exclude_pattern));
                }
            }

            return files;
        }

        static IEnumerable<string> EnumeratePattern(string raw_pattern)
        {
            var pattern_directory = Path.GetDirectoryName(raw_pattern);
            var pattern = Path.GetFileName(raw_pattern);

            string search_path;
            if (string.IsNullOrWhiteSpace(pattern_directory))
                search_path = Environment.CurrentDirectory;
            else
                search_path = Path.GetFullPath(pattern_directory);

            return Directory.EnumerateFiles(search_path, pattern, SearchOption.AllDirectories);
        }

        static string NormalizeFilename(string str)
        {
            var chars = str.ToCharArray();
            for (int i = 0; i < chars.Length; ++i)
            {
                if (chars[i] > 127) chars[i] = '-';
            }
            return new string(chars);
        }

        class IndexEntry
        {
            public string path;

            public string name;
            public uint id;
            public uint size;
            public uint offset;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2202:Do not dispose objects multiple times")]
        static void BuildMixFile(string output_path, IEnumerable<string> input_paths)
        {
            List<IndexEntry> entries = new List<IndexEntry>();

            HashSet<uint> used_ids = new HashSet<uint>();
            foreach (var path in input_paths)
            {
                var entry = new IndexEntry();
                entry.path = path;
                entry.name = NormalizeFilename(Path.GetFileName(path));
                entry.id = CRC.Compute(entry.name.ToUpper(CultureInfo.InvariantCulture));

                if (used_ids.Contains(entry.id)) throw new ArgumentOutOfRangeException();
                used_ids.Add(entry.id);

                entries.Add(entry);
            }

            entries.Sort((a, b) => a.id.CompareTo(b.id));

            using (var output_stream = new FileStream(output_path, FileMode.Create, FileAccess.Write, FileShare.None))
            using (var output_writer = new BinaryWriter(output_stream))
            {
                output_writer.Write(0x3158494D); // MIX1

                output_stream.Position = 16;
                foreach (var entry in entries)
                {
                    using (var input_stream = new FileStream(entry.path, FileMode.Open, FileAccess.Read))
                    {
                        entry.size = (uint)input_stream.Length;
                        entry.offset = (uint)output_stream.Position;
                        input_stream.CopyTo(output_stream);
                        output_stream.Position += -output_stream.Position & 7;
                    }
                }

                var index_offset = (uint)output_stream.Position;
                output_writer.Write(entries.Count);
                foreach (var entry in entries)
                {
                    output_writer.Write(entry.id);
                    output_writer.Write(entry.offset);
                    output_writer.Write(entry.size);
                }

                var filename_offset = (uint)output_stream.Position;
                output_writer.Write(entries.Count);
                foreach (var entry in entries)
                {
                    var length = entry.name.Length + 1;
                    var bytes = Encoding.ASCII.GetBytes(entry.name);
                    output_writer.Write((byte)length);
                    output_writer.Write(bytes);
                    output_writer.Write((byte)0);
                }

                output_stream.Position = 4;
                output_writer.Write(index_offset);
                output_writer.Write(filename_offset);
            }
        }

    }
}
