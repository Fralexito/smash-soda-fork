// Instalador de Phoenix Link en UN SOLO .exe, con asistente de ventanas (sin consola).
// Lo compila tools\CREAR-INSTALADOR-EXE.ps1 con el csc.exe que trae Windows (C# 5: sin interpolación ni «?.») y estos
// archivos incrustados: PhoenixLink.zip, PhoenixGlass.zip (el .exe del overlay), datos.txt (versión) e icono.ico.
// Hace lo mismo que PhoenixLink-instalar-rapido.bat: comprueba el PC, instala lo que falte (Visual C++, ViGEmBus, WebView2),
// copia Phoenix Link y el overlay y crea los accesos directos. Pide administrador (manifiesto).
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

public static class Instalador
{
    public static string LogRuta = Path.Combine(Path.GetTempPath(), "PhoenixLink-instalador.log");

    public static void Log(string texto)
    {
        try { File.AppendAllText(LogRuta, "[" + DateTime.Now.ToString("HH:mm:ss") + "] " + texto + "\r\n"); } catch { }
    }

    public static Stream Recurso(string nombre)
    {
        return Assembly.GetExecutingAssembly().GetManifestResourceStream(nombre);
    }

    public static string Dato(string clave)
    {
        try
        {
            using (StreamReader r = new StreamReader(Recurso("datos.txt")))
            {
                string linea;
                while ((linea = r.ReadLine()) != null)
                {
                    int i = linea.IndexOf('=');
                    if (i > 0 && linea.Substring(0, i).Trim() == clave) return linea.Substring(i + 1).Trim();
                }
            }
        }
        catch { }
        return "";
    }

    // ---- ¿qué falta en este PC? --------------------------------------------------------------------------
    public static bool VisualCppInstalado()
    {
        using (RegistryKey k = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64"))
            return k != null && Convert.ToInt32(k.GetValue("Installed", 0)) == 1;
    }

    public static bool ViGEmInstalado()
    {
        using (RegistryKey k = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Services\ViGEmBus")) return k != null;
    }

    public static bool WebView2Instalado()
    {
        const string guid = @"Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";
        string[] claves = { @"SOFTWARE\WOW6432Node\" + guid, @"SOFTWARE\" + guid };
        foreach (string c in claves)
            using (RegistryKey k = Registry.LocalMachine.OpenSubKey(c))
                if (k != null && k.GetValue("pv") != null) return true;
        using (RegistryKey k = Registry.CurrentUser.OpenSubKey(@"Software\" + guid))
            return k != null && k.GetValue("pv") != null;
    }

    // ---- utilidades --------------------------------------------------------------------------------------
    static void Descargar(string url, string destino, Action<int> porcentaje)
    {
        ServicePointManager.SecurityProtocol = (SecurityProtocolType)3072;   // TLS 1.2
        using (WebClient wc = new WebClient())
        {
            ManualResetEvent fin = new ManualResetEvent(false);
            Exception error = null;
            wc.DownloadProgressChanged += delegate (object s, DownloadProgressChangedEventArgs e) { if (porcentaje != null) porcentaje(e.ProgressPercentage); };
            wc.DownloadFileCompleted += delegate (object s, System.ComponentModel.AsyncCompletedEventArgs e) { error = e.Error; fin.Set(); };
            wc.DownloadFileAsync(new Uri(url), destino);
            fin.WaitOne();
            if (error != null) throw error;
        }
    }

    static int Ejecutar(string exe, string argumentos)
    {
        ProcessStartInfo psi = new ProcessStartInfo(exe, argumentos);
        psi.UseShellExecute = false;
        psi.CreateNoWindow = true;
        using (Process p = Process.Start(psi))
        {
            p.WaitForExit();
            return p.ExitCode;
        }
    }

    public static void ExtraerZip(Stream zip, string destino)
    {
        string raiz = Path.GetFullPath(destino).TrimEnd('\\') + "\\";
        using (ZipArchive a = new ZipArchive(zip, ZipArchiveMode.Read))
        {
            foreach (ZipArchiveEntry e in a.Entries)
            {
                string nombre = e.FullName.Replace('/', '\\');
                if (nombre.Length == 0 || nombre.EndsWith("\\")) { Directory.CreateDirectory(Path.Combine(destino, nombre)); continue; }
                string ruta = Path.GetFullPath(Path.Combine(destino, nombre));
                if (!ruta.StartsWith(raiz, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("Ruta no permitida en el paquete: " + nombre);
                Directory.CreateDirectory(Path.GetDirectoryName(ruta));
                e.ExtractToFile(ruta, true);
            }
        }
    }

    static void AccesoDirecto(string lnk, string destino, string carpetaTrabajo)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(lnk));
        Type t = Type.GetTypeFromProgID("WScript.Shell");
        object shell = Activator.CreateInstance(t);
        object acceso = t.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { lnk });
        Type ta = acceso.GetType();
        ta.InvokeMember("TargetPath", BindingFlags.SetProperty, null, acceso, new object[] { destino });
        ta.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, acceso, new object[] { carpetaTrabajo });
        ta.InvokeMember("IconLocation", BindingFlags.SetProperty, null, acceso, new object[] { destino });
        ta.InvokeMember("Description", BindingFlags.SetProperty, null, acceso, new object[] { "Phoenix Link" });
        ta.InvokeMember("Save", BindingFlags.InvokeMethod, null, acceso, null);
    }

    public static bool PhoenixAbierto()
    {
        return Process.GetProcessesByName("PhoenixLink").Length > 0 || Process.GetProcessesByName("PhoenixGlass").Length > 0;
    }

    // ---- pasos de verdad (los usa el asistente y las pruebas) -----------------------------------------------
    /// Copia Phoenix Link y el overlay. Devuelve el texto de lo que pasó con el overlay.
    public static void InstalarArchivos(string carpeta)
    {
        Directory.CreateDirectory(carpeta);
        using (Stream s = Recurso("PhoenixLink.zip")) ExtraerZip(s, carpeta);
        string[] imprescindibles = { "PhoenixLink.exe", "parsec.dll", "SDL2.dll", "WebView2Loader.dll", @"ui\index.html", @"sider\phoenix.lua", @"sider\phoenix_referee.lua" };
        List<string> faltan = new List<string>();
        foreach (string f in imprescindibles) if (!File.Exists(Path.Combine(carpeta, f))) faltan.Add(f);
        if (faltan.Count > 0) throw new InvalidOperationException("Faltan archivos en la instalación: " + string.Join(", ", faltan.ToArray()));

        string overlay = Path.Combine(carpeta, "overlay");
        Directory.CreateDirectory(overlay);
        Directory.CreateDirectory(Path.Combine(overlay, "plugins"));
        Directory.CreateDirectory(Path.Combine(overlay, "themes"));
        using (Stream s = Recurso("PhoenixGlass.zip")) ExtraerZip(s, overlay);
        if (!File.Exists(Path.Combine(overlay, "PhoenixGlass.exe"))) throw new InvalidOperationException("No se pudo instalar el overlay.");
    }

    public delegate void Avance(int porcentaje, string texto);

    /// Todo el proceso. Lanza excepción con un mensaje legible si algo imprescindible falla.
    public static void InstalarTodo(string carpeta, bool escritorio, Avance avance, Func<bool> cancelado)
    {
        string trabajo = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "PhoenixInstaller");
        try
        {
            if (Directory.Exists(trabajo)) Directory.Delete(trabajo, true);
            Directory.CreateDirectory(trabajo);

            avance(2, "Comprobando tu PC…");
            if (!Environment.Is64BitOperatingSystem) throw new InvalidOperationException("Phoenix Link necesita Windows de 64 bits.");
            string unidad = Path.GetPathRoot(Path.GetFullPath(carpeta));
            try { if (new DriveInfo(unidad).AvailableFreeSpace < 1024L * 1024 * 1024) throw new InvalidOperationException("Hay menos de 1 GB libre en " + unidad); }
            catch (ArgumentException) { }
            Log("instalar en " + carpeta);

            bool vc = VisualCppInstalado(), vigem = ViGEmInstalado(), wv2 = WebView2Instalado();
            Log("vc=" + vc + " vigem=" + vigem + " wv2=" + wv2);

            if (!vc)
            {
                avance(6, "Descargando el runtime de Visual C++…");
                string exe = Path.Combine(trabajo, "vc_redist.x64.exe");
                try { Descargar("https://aka.ms/vs/17/release/vc_redist.x64.exe", exe, delegate (int p) { avance(6 + p / 12, "Descargando el runtime de Visual C++… " + p + "%"); }); }
                catch (Exception e) { throw new InvalidOperationException("No se pudo bajar el runtime de Visual C++. Revisa tu internet. (" + e.Message + ")"); }
                avance(15, "Instalando el runtime de Visual C++…");
                int rc = Ejecutar(exe, "/install /quiet /norestart");
                if (rc != 0 && rc != 3010 && rc != 1638) throw new InvalidOperationException("El instalador de Visual C++ falló (código " + rc + ").");
            }
            if (cancelado()) throw new OperationCanceledException();

            if (!vigem)
            {
                avance(20, "Descargando el controlador de mandos virtuales (ViGEmBus)…");
                string exe = Path.Combine(trabajo, "ViGEmBus.exe");
                try { Descargar("https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe", exe, delegate (int p) { avance(20 + p / 12, "Descargando ViGEmBus… " + p + "%"); }); }
                catch (Exception e) { throw new InvalidOperationException("No se pudo bajar ViGEmBus. Revisa tu internet. (" + e.Message + ")"); }
                avance(30, "Instalando ViGEmBus…");
                int rc = Ejecutar(exe, "/qn");
                if (rc != 0 && rc != 3010) throw new InvalidOperationException("El instalador de ViGEmBus falló (código " + rc + ").");
            }
            if (cancelado()) throw new OperationCanceledException();

            if (!wv2)
            {
                avance(36, "Descargando el motor de la interfaz (WebView2)…");
                string exe = Path.Combine(trabajo, "WebView2Setup.exe");
                try
                {
                    Descargar("https://go.microsoft.com/fwlink/p/?LinkId=2124703", exe, delegate (int p) { avance(36 + p / 12, "Descargando WebView2… " + p + "%"); });
                    avance(45, "Instalando WebView2…");
                    Ejecutar(exe, "/silent /install");
                }
                catch (Exception e) { Log("WebView2 no se instaló: " + e.Message); /* opcional: Link usa su interfaz simple */ }
            }
            if (cancelado()) throw new OperationCanceledException();

            avance(55, "Instalando Phoenix Link…");
            try { InstalarArchivos(carpeta); }
            catch (IOException e) { throw new InvalidOperationException("No se pudieron copiar los archivos. ¿Phoenix Link está abierto? Ciérralo y vuelve a intentarlo. (" + e.Message + ")"); }
            catch (UnauthorizedAccessException e) { throw new InvalidOperationException("Sin permiso para escribir en esa carpeta. (" + e.Message + ")"); }

            avance(88, "Creando accesos directos…");
            try
            {
                string exeLink = Path.Combine(carpeta, "PhoenixLink.exe");
                string menu = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonPrograms), "Phoenix Link");
                AccesoDirecto(Path.Combine(menu, "Phoenix Link.lnk"), exeLink, carpeta);
                if (escritorio) AccesoDirecto(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory), "Phoenix Link.lnk"), exeLink, carpeta);
            }
            catch (Exception e) { Log("accesos directos: " + e.Message); }

            avance(100, "Listo.");
            Log("fin correcto");
        }
        finally
        {
            try { if (Directory.Exists(trabajo)) Directory.Delete(trabajo, true); } catch { }
        }
    }
}

// ---- barra de progreso propia (la de Windows no se puede pintar) --------------------------------------------
public class BarraPhoenix : Control
{
    int valor;
    public int Valor { get { return valor; } set { valor = Math.Max(0, Math.Min(100, value)); Invalidate(); } }
    public BarraPhoenix() { SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer, true); Height = 14; }
    protected override void OnPaint(PaintEventArgs e)
    {
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
        e.Graphics.Clear(Parent != null ? Parent.BackColor : Color.Black);
        Rectangle pista = new Rectangle(0, 0, Width - 1, Height - 1);
        using (GraphicsPath gp = Redondo(pista, Height / 2)) using (SolidBrush b = new SolidBrush(Color.FromArgb(26, 34, 70))) e.Graphics.FillPath(b, gp);
        int ancho = (int)((Width - 1) * valor / 100.0);
        if (ancho > Height)
        {
            Rectangle lleno = new Rectangle(0, 0, ancho, Height - 1);
            using (GraphicsPath gp = Redondo(lleno, Height / 2))
            using (LinearGradientBrush g = new LinearGradientBrush(lleno, Color.FromArgb(47, 224, 138), Color.FromArgb(25, 230, 255), 0f))
                e.Graphics.FillPath(g, gp);
        }
    }
    static GraphicsPath Redondo(Rectangle r, int radio)
    {
        GraphicsPath p = new GraphicsPath(); int d = radio * 2;
        p.AddArc(r.X, r.Y, d, d, 180, 90); p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90); p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        p.CloseFigure(); return p;
    }
}

// ---- el asistente ------------------------------------------------------------------------------------------
public class Asistente : Form
{
    static readonly Color Fondo = Color.FromArgb(10, 15, 38), Lateral = Color.FromArgb(14, 21, 48), Texto = Color.FromArgb(233, 239, 255),
        Tenue = Color.FromArgb(139, 149, 184), Acento = Color.FromArgb(25, 230, 255), Cesped = Color.FromArgb(47, 224, 138), Linea = Color.FromArgb(40, 52, 100);

    Panel[] paginas = new Panel[4];
    Button btnSiguiente, btnAtras, btnCancelar;
    TextBox txtCarpeta, txtLog;
    CheckBox chkEscritorio, chkAbrir;
    Label lblPaso, lblFinTitulo, lblFinTexto, lblActualizar;
    BarraPhoenix barra;
    Icon icono;
    int pagina;
    bool instalando, fallo, cancelar;
    string version;

    public Asistente()
    {
        version = Instalador.Dato("PHX_VERSION");
        try { using (Stream s = Instalador.Recurso("icono.ico")) { icono = new Icon(s, 128, 128); Icon = new Icon(Instalador.Recurso("icono.ico"), 32, 32); } } catch { }
        AutoScaleDimensions = new SizeF(96f, 96f);
        AutoScaleMode = AutoScaleMode.Dpi;
        Text = "Instalador de Phoenix Link";
        ClientSize = new Size(660, 440);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Fondo;
        ForeColor = Texto;
        Font = new Font("Segoe UI", 9.5f);
        DoubleBuffered = true;

        Panel lateral = new Panel();
        lateral.SetBounds(0, 0, 210, 440);
        lateral.BackColor = Lateral;
        lateral.Paint += PintarLateral;
        Controls.Add(lateral);

        for (int i = 0; i < paginas.Length; i++)
        {
            paginas[i] = new Panel();
            paginas[i].SetBounds(240, 30, 395, 330);
            paginas[i].BackColor = Fondo;
            Controls.Add(paginas[i]);
        }
        ConstruirBienvenida(paginas[0]);
        ConstruirCarpeta(paginas[1]);
        ConstruirProgreso(paginas[2]);
        ConstruirFin(paginas[3]);

        Panel separador = new Panel(); separador.SetBounds(210, 372, 450, 1); separador.BackColor = Linea; Controls.Add(separador);
        btnCancelar = Boton("Cancelar", 240, false); btnCancelar.Click += delegate { Cerrar(); };
        btnAtras = Boton("Atrás", 400, false); btnAtras.Click += delegate { Ir(pagina - 1); };
        btnSiguiente = Boton("Siguiente", 530, true); btnSiguiente.Click += SiguienteClick;
        Controls.AddRange(new Control[] { btnCancelar, btnAtras, btnSiguiente });
        FormClosing += AlCerrar;
        Ir(0);
    }

    Button Boton(string texto, int x, bool principal)
    {
        Button b = new Button();
        b.Text = texto; b.SetBounds(x, 388, principal ? 105 : 120, 34); b.FlatStyle = FlatStyle.Flat; b.Cursor = Cursors.Hand;
        b.Font = new Font("Segoe UI Semibold", 9.5f);
        b.FlatAppearance.BorderColor = principal ? Acento : Linea;
        b.BackColor = principal ? Acento : Lateral;
        b.ForeColor = principal ? Color.FromArgb(4, 18, 42) : Texto;
        return b;
    }

    Label Etiqueta(Panel p, string texto, int x, int y, int ancho, int alto, float tam, bool negrita, Color color)
    {
        Label l = new Label();
        l.Text = texto; l.SetBounds(x, y, ancho, alto); l.ForeColor = color; l.BackColor = Color.Transparent;
        l.Font = new Font(negrita ? "Segoe UI Semibold" : "Segoe UI", tam);
        p.Controls.Add(l);
        return l;
    }

    void ConstruirBienvenida(Panel p)
    {
        Etiqueta(p, "Bienvenido a Phoenix Link", 0, 0, 395, 40, 18f, true, Texto);
        Etiqueta(p, "Este asistente instala Phoenix Link y el overlay de partido en tu PC.", 0, 56, 395, 44, 10f, false, Tenue);
        Etiqueta(p, "Si falta algo que Link necesita (mandos virtuales, el motor de la interfaz o el runtime de Visual C++) se instala solo; en ese caso hace falta internet.", 0, 110, 395, 66, 10f, false, Tenue);
        Etiqueta(p, "Herramienta NO oficial para abrir salas en Parsec con más control de invitados y mandos.", 0, 190, 395, 44, 9f, false, Tenue);
        Etiqueta(p, "Tarda unos minutos. Pulsa Siguiente para continuar.", 0, 285, 395, 28, 10f, true, Cesped);
    }

    void ConstruirCarpeta(Panel p)
    {
        Etiqueta(p, "¿Dónde instalar?", 0, 0, 395, 40, 18f, true, Texto);
        Etiqueta(p, "Se crea la carpeta «Phoenix Link» dentro de la que elijas.", 0, 50, 395, 24, 10f, false, Tenue);
        txtCarpeta = new TextBox();
        txtCarpeta.SetBounds(0, 90, 290, 28); txtCarpeta.Text = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Phoenix Link");
        txtCarpeta.BackColor = Lateral; txtCarpeta.ForeColor = Texto; txtCarpeta.BorderStyle = BorderStyle.FixedSingle;
        txtCarpeta.TextChanged += delegate { ActualizarAviso(); };
        p.Controls.Add(txtCarpeta);
        Button examinar = new Button();
        examinar.Text = "Examinar…"; examinar.SetBounds(298, 88, 97, 30); examinar.FlatStyle = FlatStyle.Flat; examinar.BackColor = Lateral; examinar.ForeColor = Texto;
        examinar.FlatAppearance.BorderColor = Linea; examinar.Cursor = Cursors.Hand;
        examinar.Click += delegate
        {
            using (FolderBrowserDialog d = new FolderBrowserDialog())
            {
                d.Description = "Elige la carpeta donde se creará Phoenix Link";
                if (d.ShowDialog(this) == DialogResult.OK) txtCarpeta.Text = Path.Combine(d.SelectedPath, "Phoenix Link");
            }
        };
        p.Controls.Add(examinar);
        lblActualizar = Etiqueta(p, "", 0, 126, 395, 44, 9f, false, Cesped);
        chkEscritorio = new CheckBox();
        chkEscritorio.Text = "Crear un acceso directo en el escritorio"; chkEscritorio.Checked = true; chkEscritorio.SetBounds(0, 190, 395, 28);
        chkEscritorio.ForeColor = Texto; chkEscritorio.BackColor = Fondo;
        p.Controls.Add(chkEscritorio);
        Etiqueta(p, "Se instalará en tu carpeta de programas y se pedirán permisos de administrador (ya los has dado).", 0, 240, 395, 44, 9f, false, Tenue);
        ActualizarAviso();
    }

    void ActualizarAviso()
    {
        if (lblActualizar == null) return;
        bool hay = false;
        try { hay = File.Exists(Path.Combine(txtCarpeta.Text, "PhoenixLink.exe")); } catch { }
        lblActualizar.Text = hay ? "Ya hay una instalación de Phoenix Link ahí: se ACTUALIZA. Tus ajustes y datos no se tocan." : "";
    }

    void ConstruirProgreso(Panel p)
    {
        Etiqueta(p, "Instalando…", 0, 0, 395, 40, 18f, true, Texto);
        lblPaso = Etiqueta(p, "Preparando…", 0, 56, 395, 26, 10f, false, Texto);
        barra = new BarraPhoenix(); barra.SetBounds(0, 90, 395, 14); p.Controls.Add(barra);
        txtLog = new TextBox();
        txtLog.SetBounds(0, 126, 395, 180); txtLog.Multiline = true; txtLog.ReadOnly = true; txtLog.ScrollBars = ScrollBars.Vertical;
        txtLog.BackColor = Lateral; txtLog.ForeColor = Tenue; txtLog.BorderStyle = BorderStyle.None; txtLog.Font = new Font("Consolas", 8.5f);
        p.Controls.Add(txtLog);
    }

    void ConstruirFin(Panel p)
    {
        lblFinTitulo = Etiqueta(p, "Instalación terminada", 0, 0, 395, 40, 18f, true, Cesped);
        lblFinTexto = Etiqueta(p, "", 0, 56, 395, 190, 10f, false, Texto);
        chkAbrir = new CheckBox();
        chkAbrir.Text = "Abrir Phoenix Link ahora"; chkAbrir.Checked = true; chkAbrir.SetBounds(0, 270, 395, 28);
        chkAbrir.ForeColor = Texto; chkAbrir.BackColor = Fondo;
        p.Controls.Add(chkAbrir);
    }

    void PintarLateral(object sender, PaintEventArgs e)
    {
        Graphics g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias; g.InterpolationMode = InterpolationMode.HighQualityBicubic;
        using (LinearGradientBrush b = new LinearGradientBrush(new Rectangle(0, 0, 210, 440), Color.FromArgb(18, 28, 70), Color.FromArgb(8, 12, 30), 90f)) g.FillRectangle(b, 0, 0, 210, 440);
        if (icono != null) g.DrawIcon(icono, new Rectangle(57, 50, 96, 96));
        StringFormat centro = new StringFormat(); centro.Alignment = StringAlignment.Center;
        using (Font f = new Font("Segoe UI", 20f, FontStyle.Bold)) using (SolidBrush b = new SolidBrush(Texto)) g.DrawString("PHOENIX", f, b, new RectangleF(0, 160, 210, 40), centro);
        using (Font f = new Font("Segoe UI", 12f, FontStyle.Bold)) using (SolidBrush b = new SolidBrush(Acento)) g.DrawString("L  I  N  K", f, b, new RectangleF(0, 198, 210, 28), centro);
        using (Font f = new Font("Segoe UI", 9f)) using (SolidBrush b = new SolidBrush(Tenue))
            g.DrawString("Beta" + (version.Length > 0 ? " · versión " + version : ""), f, b, new RectangleF(0, 395, 210, 24), centro);
        using (Pen pen = new Pen(Color.FromArgb(60, 47, 224, 138), 2f)) g.DrawLine(pen, 40, 236, 170, 236);
    }

    // ---- navegación -------------------------------------------------------------------------------------
    void Ir(int n)
    {
        pagina = Math.Max(0, Math.Min(3, n));
        for (int i = 0; i < paginas.Length; i++) paginas[i].Visible = i == pagina;
        btnAtras.Visible = pagina == 1;
        btnCancelar.Visible = pagina < 2;
        if (pagina == 1 && txtCarpeta != null) { txtCarpeta.SelectionStart = txtCarpeta.TextLength; txtCarpeta.SelectionLength = 0; }
        btnSiguiente.Enabled = !instalando;
        btnSiguiente.Text = pagina == 0 ? "Siguiente" : pagina == 1 ? "Instalar" : pagina == 2 ? "Instalando…" : (fallo ? "Cerrar" : "Finalizar");
        if (pagina == 2) btnSiguiente.Enabled = false;
    }

    void SiguienteClick(object sender, EventArgs e)
    {
        if (pagina == 0) { Ir(1); return; }
        if (pagina == 1) { Empezar(); return; }
        if (pagina == 3)
        {
            if (!fallo && chkAbrir.Checked)
            {
                try { Process.Start(new ProcessStartInfo(Path.Combine(txtCarpeta.Text, "PhoenixLink.exe")) { WorkingDirectory = txtCarpeta.Text, UseShellExecute = true, Verb = "open" }); } catch { }
            }
            Close();
        }
    }

    void Cerrar() { Close(); }

    void AlCerrar(object sender, FormClosingEventArgs e)
    {
        if (instalando) { e.Cancel = true; MessageBox.Show(this, "Espera a que termine la instalación.", "Phoenix Link", MessageBoxButtons.OK, MessageBoxIcon.Information); }
    }

    // ---- instalar ---------------------------------------------------------------------------------------
    void Empezar()
    {
        string carpeta = txtCarpeta.Text.Trim();
        if (carpeta.Length < 4 || !Path.IsPathRooted(carpeta)) { MessageBox.Show(this, "Escribe una carpeta válida, por ejemplo C:\\Program Files\\Phoenix Link.", "Phoenix Link", MessageBoxButtons.OK, MessageBoxIcon.Warning); return; }
        while (Instalador.PhoenixAbierto())
        {
            if (MessageBox.Show(this, "Phoenix Link (o su overlay) está abierto. Ciérralo y pulsa Reintentar.", "Phoenix Link", MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning) != DialogResult.Retry) return;
        }
        instalando = true; fallo = false; cancelar = false;
        txtLog.Text = "";
        barra.Valor = 0;
        Ir(2);
        bool escritorio = chkEscritorio.Checked;
        Thread hilo = new Thread(delegate ()
        {
            string error = null;
            try
            {
                Instalador.InstalarTodo(carpeta, escritorio, delegate (int pct, string texto) { BeginInvoke(new Action(delegate { Avanzar(pct, texto); })); }, delegate () { return cancelar; });
            }
            catch (Exception ex) { error = ex.Message; Instalador.Log("ERROR: " + ex); }
            BeginInvoke(new Action(delegate { Terminar(error, carpeta); }));
        });
        hilo.IsBackground = true;
        hilo.Start();
    }

    string ultimo = "";
    void Avanzar(int pct, string texto)
    {
        barra.Valor = pct;
        lblPaso.Text = texto;
        string basico = texto.Contains("%") ? texto.Substring(0, texto.LastIndexOf('…') >= 0 ? texto.LastIndexOf('…') + 1 : texto.Length) : texto;
        if (basico != ultimo) { ultimo = basico; txtLog.AppendText(basico + "\r\n"); }
    }

    void Terminar(string error, string carpeta)
    {
        instalando = false; fallo = error != null;
        if (!fallo)
        {
            barra.Valor = 100;
            lblFinTitulo.Text = "Instalación terminada"; lblFinTitulo.ForeColor = Cesped;
            lblFinTexto.Text = "Phoenix Link está instalado en:\r\n" + carpeta + "\r\n\r\nSiguiente paso: abre Phoenix Link, inicia sesión en Parsec, ve a SYNC › Módulos del juego y pulsa INSTALAR (con PES 2021 cerrado).";
            chkAbrir.Visible = true;
        }
        else
        {
            lblFinTitulo.Text = "No se pudo terminar"; lblFinTitulo.ForeColor = Color.FromArgb(255, 77, 109);
            lblFinTexto.Text = error + "\r\n\r\nSi no sabes qué hacer, manda a Fralex este archivo:\r\n" + Instalador.LogRuta;
            chkAbrir.Visible = false;
        }
        Ir(3);
    }

    // ---- para revisar el aspecto sin instalar nada (lo usan las pruebas) -------------------------------------
    public void GuardarCaptura(int n, string png, string error)
    {
        if (n == 2) { barra.Valor = 62; lblPaso.Text = "Instalando Phoenix Link…"; txtLog.Text = "Comprobando tu PC…\r\nInstalando ViGEmBus…\r\nInstalando Phoenix Link…\r\n"; }
        if (n == 3) Terminar(error, txtCarpeta.Text);
        else Ir(n);
        // Se muestra fuera de pantalla para que los controles se pinten, y se dibuja el cliente a un PNG.
        ShowInTaskbar = false; StartPosition = FormStartPosition.Manual; Location = new Point(-4000, -4000);
        Show();
        for (int i = 0; i < 5; i++) { Application.DoEvents(); Thread.Sleep(40); }
        using (Bitmap bmp = new Bitmap(Width, Height))
        {
            DrawToBitmap(bmp, new Rectangle(0, 0, Width, Height));
            Rectangle cli = RectangleToScreen(ClientRectangle);
            using (Bitmap recorte = bmp.Clone(new Rectangle(cli.X - Left, cli.Y - Top, ClientSize.Width, ClientSize.Height), bmp.PixelFormat))
                recorte.Save(png, System.Drawing.Imaging.ImageFormat.Png);
        }
        Hide();
    }
}

public static class Programa
{
    [STAThread]
    static int Main(string[] args)
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        try { Application.Run(new Asistente()); return 0; }
        catch (Exception e)
        {
            Instalador.Log("fallo general: " + e);
            MessageBox.Show("No se pudo abrir el instalador: " + e.Message + "\r\n\r\nRegistro: " + Instalador.LogRuta, "Phoenix Link", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
}
