using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Reflection;
using System.Diagnostics;
using System.Linq;
using System.IO;
using System.Text;
using osu.Helpers;
using System.Threading;

namespace osu_beatmapfinder
{
    public struct BeatmapEntry
    {
        public string BeatmapChecksum;
        public string BeatmapFileName;
        public string FolderName;
    }
    public interface ISerializable
    {
        void ReadFromStream(SerializationReader r);
        //void WriteToStream(SerializationWriter w);
    }
    public class SerializationReader : BinaryReader
    {
        internal enum TypeBytes : byte
        {
            Null,
            Bool,
            Byte,
            UShort,
            UInt,
            ULong,
            SByte,
            Short,
            Int,
            Long,
            Char,
            String,
            Float,
            Double,
            Decimal,
            DateTime,
            ByteArray,
            CharArray,
            Unknown,
            Serializable
        }
        public SerializationReader(Stream input) : base(input) { }
        public SerializationReader(Stream input, Encoding encoding) : base(input, encoding) { }
        public SerializationReader(Stream input, Encoding encoding, bool leaveOpen) : base(input, encoding, leaveOpen) { }

        public byte[] ReadBytes()   // an overload to ReadBytes(int count)
        {
            int length = ReadInt32();
            return length > 0
                ? base.ReadBytes(length)
                : null;
        }

        public char[] ReadChars()   // an overload to ReadChars(int count)
        {
            int length = ReadInt32();
            return length > 0
                ? base.ReadChars(length)
                : null;
        }

        public override string ReadString()
        {
            switch (ReadByte())
            {
                case (byte)TypeBytes.Null:
                    return null;
                case (byte)TypeBytes.String:
                    return base.ReadString();
                default:
                    throw new Exception($"Type byte is not {TypeBytes.Null} or {TypeBytes.String} (position: {BaseStream.Position})");
            }
        }

        public DateTime ReadDateTime()
        {
            return new DateTime(ReadInt64(), DateTimeKind.Utc);
        }

        public List<T> ReadSerializableList<T>() where T : ISerializable, new()
        {
            var list = new List<T>();
            int length = ReadInt32();
            for (int i = 0; i < length; i++)
            {
                var a = new T();
                a.ReadFromStream(this);
                list.Add(a);
            }
            return list;
        }

        public List<T> ReadList<T>()
        {
            var l = new List<T>();
            int length = ReadInt32();
            for (int i = 0; i < length; i++)
                l.Add((T)ReadObject());
            return l;
        }

        public Dictionary<TKey, TValue> ReadDictionary<TKey, TValue>()
        {
            var dic = new Dictionary<TKey, TValue>();
            int length = ReadInt32();
            for (int i = 0; i < length; i++)
                dic[(TKey)ReadObject()] = (TValue)ReadObject();
            return dic;
        }

        public object ReadObject()
        {
            switch ((TypeBytes)ReadByte())
            {
                case TypeBytes.Null: return null;
                case TypeBytes.Bool: return ReadBoolean();
                case TypeBytes.Byte: return ReadByte();
                case TypeBytes.UShort: return ReadUInt16();
                case TypeBytes.UInt: return ReadUInt32();
                case TypeBytes.ULong: return ReadUInt64();
                case TypeBytes.SByte: return ReadSByte();
                case TypeBytes.Short: return ReadInt16();
                case TypeBytes.Int: return ReadInt32();
                case TypeBytes.Long: return ReadInt64();
                case TypeBytes.Char: return ReadChar();
                case TypeBytes.String: return base.ReadString();
                case TypeBytes.Float: return ReadSingle();
                case TypeBytes.Double: return ReadDouble();
                case TypeBytes.Decimal: return ReadDecimal();
                case TypeBytes.DateTime: return ReadDateTime();
                case TypeBytes.ByteArray: return ReadBytes();
                case TypeBytes.CharArray: return ReadChars();
                case TypeBytes.Unknown:
                case TypeBytes.Serializable:
                default:
                    throw new NotImplementedException();
            }
        }
    }
    class Program
    {
        public enum Mods
        {
            None = 0,
            NoFail = 1 << 0,
            Easy = 1 << 1,
            TouchDevice = 1 << 2,   //previously NoVideo
            Hidden = 1 << 3,
            HardRock = 1 << 4,
            SuddenDeath = 1 << 5,
            DoubleTime = 1 << 6,
            Relax = 1 << 7,
            HalfTime = 1 << 8,
            Nightcore = 1 << 9,
            Flashlight = 1 << 10,
            Autoplay = 1 << 11,
            SpunOut = 1 << 12,
            Relax2 = 1 << 13,  //AutoPilot
            Perfect = 1 << 14,
            Key4 = 1 << 15,
            Key5 = 1 << 16,
            Key6 = 1 << 17,
            Key7 = 1 << 18,
            Key8 = 1 << 19,
            FadeIn = 1 << 20,
            Random = 1 << 21,
            Cinema = 1 << 22,
            Target = 1 << 23,
            Key9 = 1 << 24,
            KeyCoop = 1 << 25,
            Key1 = 1 << 26,
            Key3 = 1 << 27,
            Key2 = 1 << 28,
            ScoreV2 = 1 << 29,
        }
        public struct TimingPoint : ISerializable
        {
            public double Time, MsPerQuarter;
            public bool TimingChange;

            //only in .osu files
            public int? TimingSignature; // x/4 (eg. 4/4, 3/4)
            public int? SampleSet;
            public int? CustomSampleSet;
            public int? SampleVolume;
            public bool? Kiai;
            public void ReadFromStream(SerializationReader r)
            {
                MsPerQuarter = r.ReadDouble();
                Time = r.ReadDouble();
                TimingChange = r.ReadBoolean();
            }
        }
        const int WM_COPYDATA = 0x004A;
        public struct COPYDATASTRUCT
        {
            public IntPtr dwData;
            public int cData;
            [MarshalAs(UnmanagedType.LPStr)]
            public string lpData;
        }
        public struct OsuDb
        {
            public int OsuVersion;
            public List<BeatmapEntry> Beatmaps;
        }
        static OsuDb ReadDb(string path)
        {
            OsuDb db = new OsuDb();
            using (var r = new SerializationReader(File.OpenRead(path)))
            {
                db.OsuVersion = r.ReadInt32();
                r.ReadInt32();
                r.ReadBoolean();
                r.ReadDateTime();
                r.ReadString();
                db.Beatmaps = new List<BeatmapEntry>();
                int length = r.ReadInt32();
                for (int i = 0; i < length; i++)
                {
                    int currentIndex = (int)r.BaseStream.Position;
                    int entryLength = r.ReadInt32();
                    db.Beatmaps.Add(ReadFromReader(r, false, db.OsuVersion));
                    if (r.BaseStream.Position != currentIndex + entryLength + 4)
                    {
                        Debug.Fail($"Length doesn't match, {r.BaseStream.Position} instead of expected {currentIndex + entryLength + 4}");
                    }
                }
                r.ReadByte();
            }
            return db;
        }
        public static BeatmapEntry ReadFromReader(SerializationReader r, bool readLength = true, int version = 20160729)
        {
            BeatmapEntry e = new BeatmapEntry();

            int length = 0;
            if (readLength) length = r.ReadInt32();
            int startPosition = (int)r.BaseStream.Position;

            r.ReadString();
            r.ReadString();
            r.ReadString();
            r.ReadString();
            r.ReadString();
            r.ReadString();
            r.ReadString();
            e.BeatmapChecksum = r.ReadString();
            e.BeatmapFileName = r.ReadString();
            r.ReadByte();
            r.ReadUInt16();
            r.ReadUInt16();
            r.ReadUInt16();
            r.ReadDateTime();
            if (version >= 20140609)
            {
                r.ReadSingle();
                r.ReadSingle();
                r.ReadSingle();
                r.ReadSingle();
            }
            else
            {
                r.ReadByte();
                r.ReadByte();
                r.ReadByte();
                r.ReadByte();
            }

            r.ReadDouble();

            if (version >= 20140609)
            {
                r.ReadDictionary<Mods, double>();
                r.ReadDictionary<Mods, double>();
                r.ReadDictionary<Mods, double>();
                r.ReadDictionary<Mods, double>();
            }
            r.ReadInt32();
            r.ReadInt32();
            r.ReadInt32();
            r.ReadSerializableList<TimingPoint>();
            r.ReadInt32();
            r.ReadInt32();
            r.ReadInt32();
            r.ReadByte();
            r.ReadByte();
            r.ReadByte();
            r.ReadByte();
            r.ReadInt16();
            r.ReadSingle();
            r.ReadByte();
            r.ReadString();
            r.ReadString();
            r.ReadInt16();
            r.ReadString();
            r.ReadBoolean();
            r.ReadDateTime();
            r.ReadBoolean();
            e.FolderName = r.ReadString();
            r.ReadDateTime();
            r.ReadBoolean();
            r.ReadBoolean();
            r.ReadBoolean();
            r.ReadBoolean();
            r.ReadBoolean();
            if (version < 20140609)
                r.ReadInt16();
            r.ReadInt32();
            r.ReadByte();

            int endPosition = (int)r.BaseStream.Position;
            Debug.Assert(!readLength || length == endPosition - startPosition);

            return e;
        }
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hwnd, int msg, int wParam, ref COPYDATASTRUCT IParam);
        [DllImport("User32.dll")]
        public static extern int FindWindow(string lpClassName, string lpWindowName);
        static bool CheckOsuExists() => Process.GetProcessesByName("osu!").Any();
        static bool CheckMainExists() => Process.GetProcessesByName("osu!bpm").Any();
        static Process GetOsu() => Process.GetProcessesByName("osu!").First();
        static string GetOsuFolder()
        {
            string exePath = GetOsu()?.MainModule.FileName;
            return exePath?.Substring(0, exePath.LastIndexOf(Path.DirectorySeparatorChar) + 1);
        }
        static void Main()
        {
            AppDomain.CurrentDomain.AssemblyResolve += (sender, args) =>
            new AssemblyName(args.Name).Name == "osu!" && CheckOsuExists() ? Assembly.LoadFile(GetOsu().MainModule.FileName) : null;
            var ipc = (InterProcessOsu)Activator.GetObject(typeof(InterProcessOsu), "ipc://osu!/loader");
            string osuPath = GetOsuFolder();
            var db = ReadDb(Path.Combine(osuPath, "osu!.db"));
            string hash = new string('0', 32);
            IntPtr hWnd = (IntPtr)0;
            if (CheckMainExists()) hWnd = (IntPtr)FindWindow("osubpmreceiver",null);
            //Console.Write("{0}\n", hWnd);
            while (CheckOsuExists() && CheckMainExists())
            {
                var data = ipc.GetBulkClientData();
                if (data.BeatmapChecksum != hash)
                {
                    hash = data.BeatmapChecksum;
                    var mapTemp = db.Beatmaps.FirstOrDefault(a => a.BeatmapChecksum == hash);
                    var filepath = Path.Combine(osuPath, "Songs", mapTemp.FolderName, mapTemp.BeatmapFileName);
                    //Console.Write("{0}\n", filepath);
                    byte[] arr = Encoding.Default.GetBytes(filepath);
                    int len = arr.Length;
                    COPYDATASTRUCT cdata;
                    cdata.dwData = (IntPtr)0;
                    cdata.lpData = filepath;
                    cdata.cData = len + 1;
                    SendMessage(hWnd, WM_COPYDATA, 0, ref cdata);

                }
                Thread.Sleep(50);
            }
        }
    }
}
