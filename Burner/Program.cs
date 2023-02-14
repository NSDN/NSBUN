using System.IO.Ports;

const int BLK_SIZ = 1024;

if (args.Length < 2 || args.Length > 4)
{
    Console.WriteLine("NyaSama Burner v1.0\n");
    Console.WriteLine("Usage: Burner [-F] [-E|-D] PORT [ADDR[:SIZE]] binfile");
    Console.WriteLine("  -F: FRAM mode");
    Console.WriteLine("  -E: Erase the whole chip");
    Console.WriteLine("  -D: Dump the chip");
    Console.WriteLine("  PORT: Input as \"COMx\"");
    Console.WriteLine("  ADDR: Start address, like \"0x4000\" or \"32768\"");
    Console.WriteLine("  SIZE: Region size, default is 0x8000 (32KiB)");
    Console.WriteLine();
    return;
}

Console.CancelKeyPress += (s, e) =>
{
    Console.WriteLine("\nBurner interrupted.");
};

string[] arr = args;

bool fram = false;
if (arr[0].ToLower() == "-f")
{
    fram = true;
    arr = arr[1..];
}

bool erase = false, dump = false;
if (arr[0].ToLower() == "-e")
{
    erase = true;
    arr = arr[1..];
}
else if (arr[0].ToLower() == "-d")
{
    dump = true;
    arr = arr[1..];
}

string port = arr[0];
string file = arr[arr.Length == 2 ? 1 : 2];
int addr, size;
if (arr.Length == 2)
{
    addr = 0;
    size = 0x8000;
}
else
{
    if (arr[1].Contains(':'))
    {
        string[] str = arr[1].Split(':');
        addr = str[0].StartsWith("0x") ? int.Parse(str[0][2..], System.Globalization.NumberStyles.HexNumber) : int.Parse(str[0]);
        size = str[1].StartsWith("0x") ? int.Parse(str[1][2..], System.Globalization.NumberStyles.HexNumber) : int.Parse(str[1]);
    }
    else
    {
        addr = arr[1].StartsWith("0x") ? int.Parse(arr[1][2..], System.Globalization.NumberStyles.HexNumber) : int.Parse(arr[1]);
        size = 0x8000;
    }
}

SerialPort serial;
if (port.Contains('@'))
    serial = new(port.Split('@')[0], int.Parse(port.Split('@')[1]));
else
    serial = new(port, 500000);
serial.WriteBufferSize = 4096;
serial.Open();

void write(params byte[] bytes)
{
    serial.Write(bytes, 0, bytes.Length);
}

void delay_us(int us)
{
    Task.Delay(new TimeSpan(10 * us));
}

bool wait_data(int len)
{
    DateTime start = DateTime.Now;
    while (serial.BytesToRead < len)
    {
        if ((DateTime.Now - start).TotalMilliseconds > 1000)
            return false;
    }
    return true;
}

if (serial.IsOpen)
{
    if (dump)
    {
        Console.WriteLine($"Dumping 0x{addr:X4}:0x{size:X4} to \"{file}\"...");

        if (fram)
        {
            Console.WriteLine("Switching to FRAM mode...");
            write(0x99, 0x66, 0x66);
        }

        List<byte> byteBuff = new();

        ushort chksum; int len;
        int blk;
        for (blk = 0; blk < size / BLK_SIZ; blk++)
        {
            uint a = (uint)(addr + blk * BLK_SIZ);
            write(0xA5, (byte)(a & 0xFF), (byte)(a >> 8));

            if ((len = size - blk * BLK_SIZ) < BLK_SIZ)
            {
                write(0xAB, 0xAA, 0x55);
                if (serial.ReadLine() == "GO!")
                {
                    Console.Write($"Loading...{100.0 * blk / (size / BLK_SIZ):00.00}%\r");
                    byte[] buff = new byte[BLK_SIZ + 2];
                    if (wait_data(buff.Length) && serial.Read(buff, 0, buff.Length) == buff.Length)
                    {
                        chksum = 0;
                        for (int j = 0; j < BLK_SIZ; j++)
                            chksum += buff[j];
                        if ((byte)(chksum & 0xFF) == buff[^2] && (byte)(chksum >> 8) == buff[^1])
                        {
                            byteBuff.AddRange(buff[..len]);
                            continue;
                        }
                    }

                    Console.WriteLine("\nError!");
                    break;
                }
            }
            else
            {
                write(0xAB, 0xAA, 0x55);
                if (serial.ReadLine() == "GO!")
                {
                    Console.Write($"Loading...{100.0 * blk / (size / BLK_SIZ):00.00}%\r");
                    byte[] buff = new byte[BLK_SIZ + 2];
                    if (wait_data(buff.Length) && serial.Read(buff, 0, buff.Length) == buff.Length)
                    {
                        chksum = 0;
                        for (int j = 0; j < BLK_SIZ; j++)
                            chksum += buff[j];
                        if ((byte)(chksum & 0xFF) == buff[^2] && (byte)(chksum >> 8) == buff[^1])
                        {
                            byteBuff.AddRange(buff[..^2]);
                            continue;
                        }
                    }

                    Console.WriteLine("\nError!");
                    break;
                }
            }
        }

        if (blk == size / BLK_SIZ)
        {
            File.WriteAllBytes(file, byteBuff.ToArray());
            Console.WriteLine($"Burner has transfered {size / 1024.0:0.0}KiB.");
        }
    }
    else
    {
        Console.WriteLine($"Writing \"{file}\" to 0x{addr:X4}...");

        if (fram)
        {
            Console.WriteLine("Switching to FRAM mode...");
            write(0x99, 0x66, 0x66);
        }
        else
        {
            write(0x99, 0x00, 0x00);

            Console.Write("Disabling SDP...");
            write(0x59, 0x55, 0x55);
            if (serial.ReadLine() == "OK!")
            {
                Console.WriteLine("OK");
            }
        }
        if (erase)
        {
            Console.WriteLine("Erasing...");
            write(0x55, 0x32, 0x32);
        }
        if (!erase || serial.ReadLine() == "OK!")
        {
            Console.WriteLine("Loading...");
            byte[] data = File.ReadAllBytes(file);

            ushort chksum; int len;
            int blk;
            for (blk = 0; blk < data.Length / BLK_SIZ; blk++)
            {
                uint a = (uint)(addr + blk * BLK_SIZ);
                write(0xA5, (byte)(a & 0xFF), (byte)(a >> 8));

                if ((len = data.Length - blk * BLK_SIZ) < BLK_SIZ)
                {
                    chksum = 0;
                    for (int j = 0; j < len; j++)
                        chksum += data[blk * BLK_SIZ + j];
                    write(0xA9, (byte)(chksum & 0xFF), (byte)(chksum >> 8));

                    write(0xAA, 0x55, 0xAA);
                    if (serial.ReadLine() == "GO!")
                    {
                        Console.Write($"Writing...{100.0 * blk / (data.Length / BLK_SIZ):00.00}%\r");
                        for (int j = 0; j < len; j++)
                            write(data[blk * BLK_SIZ + j]);
                        for (int j = 0; j < BLK_SIZ - len; j++)
                            write(0x00);

                        while (serial.BytesToWrite > 0)
                            delay_us(10);
                        if (serial.ReadLine() == "ERR")
                        {
                            Console.WriteLine("\nError!");
                            break;
                        }
                    }
                }
                else
                {
                    chksum = 0;
                    for (int j = 0; j < BLK_SIZ; j++)
                        chksum += data[blk * BLK_SIZ + j];
                    write(0xA9, (byte)(chksum & 0xFF), (byte)(chksum >> 8));

                    write(0xAA, 0x55, 0xAA);
                    if (serial.ReadLine() == "GO!")
                    {
                        Console.Write($"Writing...{100.0 * blk / (data.Length / BLK_SIZ):00.00}%\r");
                        for (int j = 0; j < BLK_SIZ; j++)
                            write(data[blk * BLK_SIZ + j]);

                        while (serial.BytesToWrite > 0)
                            delay_us(10);
                        if (serial.ReadLine() == "ERR")
                        {
                            Console.WriteLine("\nError!");
                            break;
                        }
                    }
                }
            }

            if (!fram)
            {
                Console.Write("Enabling SDP...");
                write(0x59, 0x00, 0x00);
                if (serial.ReadLine() == "OK!")
                {
                    Console.WriteLine("OK");
                }
            }

            if (blk == data.Length / BLK_SIZ)
                Console.WriteLine($"Burner has transfered {data.Length / 1024.0:0.0}KiB.");
        }

        serial.Close();
    }
}
