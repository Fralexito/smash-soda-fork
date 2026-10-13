// Lanzador del instalador de Phoenix Link en un solo .exe.
// Lo compila tools\CREAR-INSTALADOR-EXE.ps1 con el csc.exe que trae Windows, con estos archivos incrustados:
//   instalador.bat (PhoenixLink-instalar-rapido.bat), PhoenixLink.zip, PhoenixGlass.zip, datos.txt (SHA256 de cada uno).
// Pide permisos de administrador (manifiesto), vuelca todo a una carpeta temporal, ejecuta el .bat apuntando a los
// zips locales (no baja Phoenix Link de internet; solo los componentes de Microsoft/ViGEm si faltan) y limpia al terminar.
// «PhoenixLink-Setup.exe /extraer <carpeta>» solo vuelca los archivos (para revisar el contenido sin instalar nada).
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;

static class Programa
{
    static int Main(string[] args)
    {
        bool soloExtraer = args.Length >= 2 && args[0] == "/extraer";
        string dir = soloExtraer ? args[1] : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "PhoenixLinkSetup-" + Guid.NewGuid().ToString("N").Substring(0, 8));
        try
        {
            Directory.CreateDirectory(dir);
            Assembly asm = Assembly.GetExecutingAssembly();
            foreach (string nombre in asm.GetManifestResourceNames())
            {
                using (Stream s = asm.GetManifestResourceStream(nombre))
                using (FileStream f = File.Create(Path.Combine(dir, nombre)))
                    s.CopyTo(f);
            }
            if (soloExtraer) return 0;

            var datos = new Dictionary<string, string>();
            foreach (string linea in File.ReadAllLines(Path.Combine(dir, "datos.txt")))
            {
                int i = linea.IndexOf('=');
                if (i > 0) datos[linea.Substring(0, i).Trim()] = linea.Substring(i + 1).Trim();
            }

            var psi = new ProcessStartInfo("cmd.exe", "/c \"\"" + Path.Combine(dir, "instalador.bat") + "\"\"");
            psi.UseShellExecute = false;
            psi.WorkingDirectory = dir;
            psi.EnvironmentVariables["PHX_ZIP_URL"] = new Uri(Path.Combine(dir, "PhoenixLink.zip")).AbsoluteUri;
            psi.EnvironmentVariables["PHX_ZIP_SHA"] = datos["PHX_ZIP_SHA"];
            psi.EnvironmentVariables["PHX_OVERLAY_URL"] = new Uri(Path.Combine(dir, "PhoenixGlass.zip")).AbsoluteUri;
            psi.EnvironmentVariables["PHX_OVERLAY_SHA"] = datos["PHX_OVERLAY_SHA"];
            psi.EnvironmentVariables["PHX_OVERLAY_EXE_SHA"] = datos["PHX_OVERLAY_EXE_SHA"];
            using (Process p = Process.Start(psi))
            {
                p.WaitForExit();
                return p.ExitCode;
            }
        }
        catch (Exception e)
        {
            Console.Error.WriteLine("No se pudo iniciar el instalador: " + e.Message);
            return 1;
        }
        finally
        {
            if (!soloExtraer)
            {
                try { Directory.Delete(dir, true); } catch { }
            }
        }
    }
}
