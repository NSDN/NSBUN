using System.IO.Ports;

const int BLK_SIZ = 1024;

if (args.Length < 2 || args.Length > 4)
{
    Console.WriteLine("RomLoader v1.0\n");
    Console.WriteLine("Usage: RomLoader [-F] [-E] PORT [ADDR] binfile");
    Console.WriteLine("  -F: FRAM mode");
    Console.WriteLine("  -E: Erase the whole chip");
    Console.WriteLine("  PORT: Input as \"COMx\"");
    Console.WriteLine("  ADDR: Start address, like \"0x4000\" or \"32768\"");
    Console.WriteLine();
    return;
}

Console.CancelKeyPress += (s, e) =>
{
    Console.WriteLine("\nRomLoader interrupted.");
};

string[] arr = args;

bool fram = false;
if (arr[0].ToLower() == "-f")
{
    fram = true;
    arr = arr[1..];
}

bool erase = false;
if (arr[0].ToLower() == "-e")
{
    erase = true;
    arr = arr[1..];
}

string port = arr[0];
string file = arr[arr.Length == 2 ? 1 : 2];
int addr = arr.Length == 2 ? 0 : (arr[1].StartsWith("0x") ? int.Parse(arr[1][2..], System.Globalization.NumberStyles.HexNumber) : int.Parse(arr[1]));

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

if (serial.IsOpen)
{
    Console.WriteLine($"Writing \"{file}\" to 0x{addr:X6}...");

    if (fram) {
        Console.WriteLine("Switching to FRAM mode...");
        write(0x99, 0x66, 0x66);
    } else {
        write(0x99, 0x00, 0x00);

        Console.Write("Disabling SDP...");
        write(0x59, 0x55, 0x55);
        if (serial.ReadLine() == "OK!") {
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

        if (!fram) {
            Console.Write("Enabling SDP...");
            write(0x59, 0x00, 0x00);
            if (serial.ReadLine() == "OK!") {
                Console.WriteLine("OK");
            }
        }

        if (blk == data.Length / BLK_SIZ)
            Console.WriteLine($"RomLoader has transfered {data.Length / 1024.0:0.0}KiB.");
    }

    serial.Close();
}
