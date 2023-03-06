using System.Runtime.InteropServices;

namespace mobiconvertnettest
{
    internal class Program
    {
        [DllImport("mobiconvert.dll", SetLastError = true)]
        static extern int ConvertMobiToEpub(
            [In] byte[] lpBuffer, 
            long byteInBuffer,
            [Out] out IntPtr lpOutBuffer, 
            out long lpNumberOfBytesRead);

        static void Main(string[] args)
        {
            var bytes = File.ReadAllBytes("test.azw3");

            IntPtr outBuffer;
            long outputLength;
            var k = ConvertMobiToEpub(bytes, bytes.Length, out outBuffer, out outputLength);
            byte[] managedArray = new byte[outputLength];
            Marshal.Copy(outBuffer, managedArray, 0, (int)outputLength);
            Marshal.FreeHGlobal(outBuffer);

            File.WriteAllBytes("test.epub", managedArray);
        }
    }
}