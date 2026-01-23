using System.Windows;
using System;

namespace FirstEngineEditor
{
    public partial class App : System.Windows.Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            System.Diagnostics.Debug.WriteLine("OnStartup called!");
            Console.WriteLine("FirstEngine Editor starting...");
            
            // Enable RenderDoc support for frame capture
            // This allows RenderDoc to work even in debug mode
            Environment.SetEnvironmentVariable("FIRSTENGINE_ENABLE_RENDERDOC", "1");
            Console.WriteLine("RenderDoc support enabled (set FIRSTENGINE_ENABLE_RENDERDOC=1)");
            
            base.OnStartup(e);
            
            // Initialize services
            Services.ServiceLocator.Initialize();
            
            // Try to auto-open project if Package directory exists in workspace root
            // This allows the editor to automatically work with existing Package structure
            try
            {
                string exeDir = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) ?? "";
                // Try to find workspace root (go up from Editor/bin/Debug/net8.0-windows)
                // Also try the actual workspace directory
                string[] possibleWorkspacePaths = {
                    System.IO.Path.Combine(exeDir, "..", "..", "..", "..", ".."),  // From Editor/bin/Debug/net8.0-windows -> workspace root
                    System.IO.Path.Combine(exeDir, "..", "..", "..", ".."),        // From Editor/bin/Debug -> workspace root
                    System.IO.Path.GetFullPath("."),                                // Current working directory
                    @"G:\AIHUMAN\WorkSpace\FirstEngine"                            // Explicit workspace path (for development)
                };
                
                var projectManager = Services.ServiceLocator.Get<Services.IProjectManager>();
                
                foreach (var possiblePath in possibleWorkspacePaths)
                {
                    try
                    {
                        string normalizedPath = System.IO.Path.GetFullPath(possiblePath);
                        string packagePath = System.IO.Path.Combine(normalizedPath, "Package");
                        
                        System.Diagnostics.Debug.WriteLine($"Checking for Package at: {packagePath}");
                        
                        if (System.IO.Directory.Exists(packagePath))
                        {
                            System.Diagnostics.Debug.WriteLine($"Found Package directory at: {packagePath}");
                            Console.WriteLine($"Found Package directory at: {packagePath}");
                            
                            // Check if project.json exists, if not create it
                            string projectJsonPath = System.IO.Path.Combine(normalizedPath, "project.json");
                            
                            if (System.IO.File.Exists(projectJsonPath))
                            {
                                bool opened = projectManager.OpenProject(normalizedPath);
                                System.Diagnostics.Debug.WriteLine($"Auto-opened project at: {normalizedPath}, success: {opened}");
                                Console.WriteLine($"Auto-opened project at: {normalizedPath}, success: {opened}");
                                if (opened) break;
                            }
                            else
                            {
                                // Create project.json for existing Package
                                string projectName = System.IO.Path.GetFileName(normalizedPath);
                                bool created = projectManager.CreateProject(normalizedPath, projectName);
                                System.Diagnostics.Debug.WriteLine($"Created project.json for existing Package at: {normalizedPath}, success: {created}");
                                Console.WriteLine($"Created project.json for existing Package at: {normalizedPath}, success: {created}");
                                if (created) break;
                            }
                        }
                    }
                    catch (Exception pathEx)
                    {
                        System.Diagnostics.Debug.WriteLine($"Error checking path {possiblePath}: {pathEx.Message}");
                        continue;
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error auto-opening project: {ex.Message}");
                Console.WriteLine($"Error auto-opening project: {ex.Message}");
                // Don't fail startup if auto-open fails
            }
        }

        protected override void OnExit(ExitEventArgs e)
        {
            // Cleanup services
            Services.ServiceLocator.Shutdown();
            
            // Shutdown global render engine
            Services.RenderEngineService.ShutdownGlobalEngine();
            
            base.OnExit(e);
        }
    }
}
