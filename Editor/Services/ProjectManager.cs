using System.IO;
using System.Text;
using FirstEngineEditor.Models;

namespace FirstEngineEditor.Services
{
    public class ProjectManager : IProjectManager
    {
        private Project? _currentProject;
        private const string ConfigFileName = "engine.ini";

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
                    
                    // Update engine.ini configuration file with Package path
                    UpdateEngineConfig(packageDir);
                    
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

                // Update engine.ini configuration file with Package path
                UpdateEngineConfig(packageDir);

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

        /// <summary>
        /// 更新 engine.ini 配置文件，设置 Package 路径
        /// </summary>
        private void UpdateEngineConfig(string packagePath)
        {
            try
            {
                // 获取配置文件路径（在可执行文件目录）
                string configPath = GetConfigFilePath();
                
                // 读取现有配置（如果存在）
                var config = new Dictionary<string, string>();
                if (File.Exists(configPath))
                {
                    foreach (var line in File.ReadAllLines(configPath))
                    {
                        var trimmedLine = line.Trim();
                        // 跳过注释和空行
                        if (string.IsNullOrEmpty(trimmedLine) || trimmedLine.StartsWith("#") || trimmedLine.StartsWith(";"))
                            continue;
                        
                        var parts = trimmedLine.Split(new[] { '=' }, 2);
                        if (parts.Length == 2)
                        {
                            config[parts[0].Trim()] = parts[1].Trim();
                        }
                    }
                }

                // 更新 Package 路径（使用绝对路径）
                string absolutePackagePath = Path.GetFullPath(packagePath);
                // 规范化路径分隔符为 /
                absolutePackagePath = absolutePackagePath.Replace('\\', '/');
                config["PackagePath"] = absolutePackagePath;

                // 写入配置文件
                var sb = new StringBuilder();
                sb.AppendLine("# FirstEngine Configuration File");
                sb.AppendLine("# This file is automatically generated. Do not edit manually.");
                sb.AppendLine();
                
                foreach (var kvp in config)
                {
                    sb.AppendLine($"{kvp.Key}={kvp.Value}");
                }

                // 确保目录存在
                string configDir = Path.GetDirectoryName(configPath);
                if (!string.IsNullOrEmpty(configDir) && !Directory.Exists(configDir))
                {
                    Directory.CreateDirectory(configDir);
                }

                File.WriteAllText(configPath, sb.ToString());
                System.Diagnostics.Debug.WriteLine($"UpdateEngineConfig: Updated engine.ini with PackagePath: {absolutePackagePath}");
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"UpdateEngineConfig: Error updating config file: {ex.Message}");
            }
        }

        /// <summary>
        /// 获取 engine.ini 配置文件路径
        /// </summary>
        private string GetConfigFilePath()
        {
            // 尝试在可执行文件目录查找
            string exeDir = AppDomain.CurrentDomain.BaseDirectory;
            string configPath = Path.Combine(exeDir, ConfigFileName);
            
            // 如果不存在，尝试在工作目录
            if (!File.Exists(configPath))
            {
                string workingDir = Directory.GetCurrentDirectory();
                configPath = Path.Combine(workingDir, ConfigFileName);
            }

            return configPath;
        }

        /// <summary>
        /// 获取上一次加载的 Package 路径（从配置文件）
        /// </summary>
        public static string? GetLastPackagePath()
        {
            try
            {
                // 尝试在可执行文件目录查找
                string exeDir = AppDomain.CurrentDomain.BaseDirectory;
                string configPath = Path.Combine(exeDir, ConfigFileName);
                
                // 如果不存在，尝试在工作目录
                if (!File.Exists(configPath))
                {
                    string workingDir = Directory.GetCurrentDirectory();
                    configPath = Path.Combine(workingDir, ConfigFileName);
                }

                if (!File.Exists(configPath))
                    return null;

                foreach (var line in File.ReadAllLines(configPath))
                {
                    var trimmedLine = line.Trim();
                    // 跳过注释和空行
                    if (string.IsNullOrEmpty(trimmedLine) || trimmedLine.StartsWith("#") || trimmedLine.StartsWith(";"))
                        continue;
                    
                    var parts = trimmedLine.Split(new[] { '=' }, 2);
                    if (parts.Length == 2 && parts[0].Trim() == "PackagePath")
                    {
                        string packagePath = parts[1].Trim();
                        // 检查路径是否存在
                        if (Directory.Exists(packagePath))
                        {
                            return packagePath;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"GetLastPackagePath: Error reading config file: {ex.Message}");
            }

            return null;
        }
    }

    public class ProjectData
    {
        public string? Name { get; set; }
        public string? Version { get; set; }
    }
}
