// =============================================================================
//  Phoenix Evolution · captura de la ventana de PES con Windows Graphics Capture (Windows 10 1903+)
//  La usa ayudante.ps1 cuando PES NO está al frente (detrás de otras ventanas): PrintWindow da en DX11
//  un fotograma viejo, y esta API copia lo último que el juego dibujó. Con PES minimizado no hay
//  imagen (el juego no dibuja) y se avisa. Viene con Windows: no se descarga nada.
//  Se compila una vez (ayudante.ps1, Add-Type con las referencias de WinMetadata) en captura_wgc.dll.
//  C# 5 (el compilador de .NET Framework que trae Windows): nada de «out var» ni $"".
// =============================================================================
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Graphics.Capture;
using Windows.Graphics.DirectX;
using Windows.Graphics.DirectX.Direct3D11;
using Windows.Graphics.Imaging;

public static class PhxWgc {
  [ComImport, Guid("3628E81B-3CAC-4C60-B7F4-23CE0E0C3356"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  interface IGraphicsCaptureItemInterop {
    IntPtr CreateForWindow([In] IntPtr ventana, [In] ref Guid iid);
    IntPtr CreateForMonitor([In] IntPtr monitor, [In] ref Guid iid);
  }
  [ComImport, Guid("905A0FEF-BC53-11DF-8C49-001E4FC686DA"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  interface IBufferByteAccess { IntPtr Buffer(); }
  [DllImport("d3d11.dll")]
  static extern int D3D11CreateDevice(IntPtr adaptador, int tipo, IntPtr software, uint flags, IntPtr niveles, uint nNiveles, uint sdk, out IntPtr dispositivo, out int nivel, out IntPtr contexto);
  [DllImport("d3d11.dll")]
  static extern int CreateDirect3D11DeviceFromDXGIDevice(IntPtr dxgi, out IntPtr inspectable);

  static readonly Guid IID_ITEM = new Guid("79C3F95B-31F7-4EC2-A464-632EF5D30760");
  static readonly Guid IID_DXGI_DEVICE = new Guid("54EC77FA-1377-44E6-8C32-88FD5F44C84C");

  public static bool Disponible() { return GraphicsCaptureSession.IsSupported(); }

  // Devuelve la imagen de la ventana o lanza una excepción con el motivo.
  public static Bitmap Capturar(IntPtr ventana, int esperaMs) {
    if (!GraphicsCaptureSession.IsSupported()) throw new Exception("este Windows no tiene Windows Graphics Capture (hace falta Windows 10 1903 o más nuevo)");
    // 1) El «objeto a capturar» a partir de la ventana.
    IGraphicsCaptureItemInterop interop = (IGraphicsCaptureItemInterop)WindowsRuntimeMarshal.GetActivationFactory(typeof(GraphicsCaptureItem));
    Guid iid = IID_ITEM;
    IntPtr pItem = interop.CreateForWindow(ventana, ref iid);
    GraphicsCaptureItem item = (GraphicsCaptureItem)Marshal.GetObjectForIUnknown(pItem);
    Marshal.Release(pItem);
    // 2) Un dispositivo Direct3D 11 propio (no toca el del juego).
    IntPtr d3d, ctx; int nivel;
    int hr = D3D11CreateDevice(IntPtr.Zero, 1, IntPtr.Zero, 0x20, IntPtr.Zero, 0, 7, out d3d, out nivel, out ctx);
    if (hr < 0) throw new Exception("no se pudo crear el dispositivo Direct3D 11 (0x" + hr.ToString("X8") + ")");
    IntPtr dxgi, pIns;
    Guid iidDxgi = IID_DXGI_DEVICE;
    Marshal.QueryInterface(d3d, ref iidDxgi, out dxgi);
    hr = CreateDirect3D11DeviceFromDXGIDevice(dxgi, out pIns);
    Marshal.Release(dxgi);
    if (hr < 0) { Marshal.Release(ctx); Marshal.Release(d3d); throw new Exception("CreateDirect3D11DeviceFromDXGIDevice falló (0x" + hr.ToString("X8") + ")"); }
    IDirect3DDevice disp = (IDirect3DDevice)Marshal.GetObjectForIUnknown(pIns);
    Marshal.Release(pIns);
    Direct3D11CaptureFramePool pool = null; GraphicsCaptureSession sesion = null; Direct3D11CaptureFrame cuadro = null;
    try {
      // 3) Pedir un fotograma.
      pool = Direct3D11CaptureFramePool.CreateFreeThreaded(disp, DirectXPixelFormat.B8G8R8A8UIntNormalized, 1, item.Size);
      sesion = pool.CreateCaptureSession(item);
      try { sesion.IsCursorCaptureEnabled = false; } catch { /* Windows 10 antiguo: sin esa opción */ }
      sesion.StartCapture();
      DateTime fin = DateTime.Now.AddMilliseconds(esperaMs);
      while (cuadro == null && DateTime.Now < fin) { cuadro = pool.TryGetNextFrame(); if (cuadro == null) System.Threading.Thread.Sleep(15); }
      if (cuadro == null) throw new Exception("el juego no entregó ninguna imagen en " + esperaMs + " ms (¿minimizado o en pausa de dibujo?)");
      // 4) De la GPU a bytes BGRA, y de ahí a un Bitmap.
      // Sin AsTask ni ToArray: esos ayudantes de .NET piden el Windows.winmd «todo en uno», que Windows 11 ya no trae
      // (probado el 29-oct). Se espera la operación mirando su estado y se leen los bytes con IBufferByteAccess.
      Windows.Foundation.IAsyncOperation<SoftwareBitmap> op = SoftwareBitmap.CreateCopyFromSurfaceAsync(cuadro.Surface, BitmapAlphaMode.Ignore);
      DateTime fin2 = DateTime.Now.AddMilliseconds(3000);
      while (op.Status == Windows.Foundation.AsyncStatus.Started && DateTime.Now < fin2) System.Threading.Thread.Sleep(5);
      if (op.Status != Windows.Foundation.AsyncStatus.Completed) throw new Exception("no se pudo copiar la imagen de la GPU (" + op.Status + ")");
      SoftwareBitmap sb = op.GetResults();
      int w = sb.PixelWidth, h = sb.PixelHeight;
      Windows.Storage.Streams.Buffer buf = new Windows.Storage.Streams.Buffer((uint)(w * h * 4));
      sb.CopyToBuffer(buf);
      byte[] px = new byte[buf.Length];
      Marshal.Copy(((IBufferByteAccess)(object)buf).Buffer(), px, 0, px.Length);
      sb.Dispose();
      int paso = h > 0 ? px.Length / h : w * 4;
      Bitmap bmp = new Bitmap(w, h, PixelFormat.Format32bppRgb);
      BitmapData bd = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppRgb);
      for (int y = 0; y < h; y++) Marshal.Copy(px, y * paso, new IntPtr(bd.Scan0.ToInt64() + (long)y * bd.Stride), w * 4);
      bmp.UnlockBits(bd);
      return bmp;
    } finally {
      if (cuadro != null) cuadro.Dispose();
      if (sesion != null) sesion.Dispose();
      if (pool != null) pool.Dispose();
      disp.Dispose();
      Marshal.Release(ctx); Marshal.Release(d3d);
    }
  }
}
