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

        public bool OpenProject(string projectPath)
        {
            if (!Directory.Exists(projectPath))
                return false;

            var projectFile = Path.Combine(projectPath, "project.json");
            if (!File.Exists(projectFile))
            {
                // Try to create a default project
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
                    return true;
                }
            }
            catch
            {
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

                // Create default directories
                Directory.CreateDirectory(Path.Combine(projectPath, "Scenes"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Models"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Materials"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Textures"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Meshes"));
                Directory.CreateDirectory(Path.Combine(projectPath, "Shaders"));

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
