using System.IO;
using FirstEngineEditor.Models;

namespace FirstEngineEditor.Services
{
    public class ProjectManager : IProjectManager
    {
        private Project? _currentProject;

        public Project? CurrentProject
        {
            get => _currentProject;
            private set
            {
                _currentProject = value;
                ProjectChanged?.Invoke(this, EventArgs.Empty);
            }
        }

        public event EventHandler? ProjectChanged;

        /// <summary>
        /// 获取Package目录的路径。智能检测Package目录位置：
        /// 1. 如果projectPath本身就是Package目录（包含resource_manifest.json），直接返回
        /// 2. 否则，返回projectPath/Package
        /// </summary>
        public static string? GetPackagePath(string? projectPath)
        {
            if (string.IsNullOrEmpty(projectPath) || !Directory.Exists(projectPath))
                return null;

            // 检查projectPath本身是否是Package目录
            string manifestInProjectPath = Path.Combine(projectPath, "resource_manifest.json");
            if (File.Exists(manifestInProjectPath))
            {
                // projectPath本身就是Package目录
                return projectPath;
            }

            // 检查projectPath/Package是否存在
            string packagePath = Path.Combine(projectPath, "Package");
            if (Directory.Exists(packagePath))
            {
                return packagePath;
            }

            return null;
        }

        public bool OpenProject(string projectPath)
        {
            if (!Directory.Exists(projectPath))
            {
                System.Diagnostics.Debug.WriteLine($"OpenProject: Directory does not exist: {projectPath}");
                return false;
            }

            var projectFile = Path.Combine(projectPath, "project.json");
            if (!File.Exists(projectFile))
            {
                // Try to create a default project
                System.Diagnostics.Debug.WriteLine($"OpenProject: project.json not found, creating default project");
                return CreateProject(projectPath, Path.GetFileName(projectPath));
            }

            try
            {
                var projectData = System.Text.Json.JsonSerializer.Deserialize<ProjectData>(
                    File.ReadAllText(projectFile));
                
                if (projectData != null)
                {
                    CurrentProject = new Project
                    {
                        Name = projectData.Name ?? Path.GetFileName(projectPath),
                        ProjectPath = projectPath
                    };
                    
                    System.Diagnostics.Debug.WriteLine($"OpenProject: Successfully opened project: {CurrentProject.Name} at {CurrentProject.ProjectPath}");
                    
                    // Check if Package directory exists, if not, create it
                    string packageDir = Path.Combine(projectPath, "Package");
                    if (!Directory.Exists(packageDir))
                    {
                        System.Diagnostics.Debug.WriteLine($"OpenProject: Package directory does not exist, creating it");
                        Directory.CreateDirectory(Path.Combine(packageDir, "Scenes"));
                        Directory.CreateDirectory(Path.Combine(packageDir, "Models"));
                        Directory.CreateDirectory(Path.Combine(packageDir, "Materials"));
                        Directory.CreateDirectory(Path.Combine(packageDir, "Textures"));
                        Directory.CreateDirectory(Path.Combine(packageDir, "Meshes"));
                        Directory.CreateDirectory(Path.Combine(packageDir, "Shaders"));
                    }
                    
                    return true;
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"OpenProject: Error opening project: {ex.Message}");
                return false;
            }

            return false;
        }

        public void CloseProject()
        {
            CurrentProject = null;
        }

        public bool CreateProject(string projectPath, string projectName)
        {
            try
            {
                if (!Directory.Exists(projectPath))
                    Directory.CreateDirectory(projectPath);

                var projectData = new ProjectData
                {
                    Name = projectName,
                    Version = "1.0.0"
                };

                var projectFile = Path.Combine(projectPath, "project.json");
                File.WriteAllText(projectFile, 
                    System.Text.Json.JsonSerializer.Serialize(projectData, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));

                // Create Package directory structure
                string packageDir = Path.Combine(projectPath, "Package");
                Directory.CreateDirectory(Path.Combine(packageDir, "Scenes"));
                Directory.CreateDirectory(Path.Combine(packageDir, "Models"));
                Directory.CreateDirectory(Path.Combine(packageDir, "Materials"));
                Directory.CreateDirectory(Path.Combine(packageDir, "Textures"));
                Directory.CreateDirectory(Path.Combine(packageDir, "Meshes"));
                Directory.CreateDirectory(Path.Combine(packageDir, "Shaders"));
                
                // Create initial resource manifest
                string manifestPath = Path.Combine(packageDir, "resource_manifest.json");
                var initialManifest = new
                {
                    version = 1,
                    nextID = 5000,
                    resources = new object[0]
                };
                File.WriteAllText(manifestPath, 
                    System.Text.Json.JsonSerializer.Serialize(initialManifest, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));

                CurrentProject = new Project
                {
                    Name = projectName,
                    ProjectPath = projectPath
                };

                return true;
            }
            catch
            {
                return false;
            }
        }

        public bool SaveProject()
        {
            if (CurrentProject == null)
                return false;

            try
            {
                var projectData = new ProjectData
                {
                    Name = CurrentProject.Name,
                    Version = "1.0.0"
                };

                var projectFile = Path.Combine(CurrentProject.ProjectPath, "project.json");
                File.WriteAllText(projectFile,
                    System.Text.Json.JsonSerializer.Serialize(projectData, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));

                return true;
            }
            catch
            {
                return false;
            }
        }
    }

    public class ProjectData
    {
        public string? Name { get; set; }
        public string? Version { get; set; }
    }
}
