using System.Collections.ObjectModel;
using System.IO;
using FirstEngineEditor.Models;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;
using Newtonsoft.Json;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class ResourceListViewModel : ViewModelBase
    {
        private readonly IProjectManager _projectManager;
        private readonly IFileManager _fileManager;
        private ResourceItem? _selectedResource;

        public ResourceListViewModel()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
            _fileManager = ServiceLocator.Get<IFileManager>();

            Resources = new ObservableCollection<ResourceItem>();
            
            _projectManager.ProjectChanged += OnProjectChanged;
            RefreshResources();
        }

        public ObservableCollection<ResourceItem> Resources { get; }

        public ResourceItem? SelectedResource
        {
            get => _selectedResource;
            set
            {
                SetProperty(ref _selectedResource, value);
                OnResourceSelected(value);
            }
        }

        private void OnProjectChanged(object? sender, System.EventArgs e)
        {
            RefreshResources();
        }

        public void RefreshResources()
        {
            Resources.Clear();

            var project = _projectManager.CurrentProject;
            if (project == null) return;

            // 使用统一的路径解析方法获取Package路径
            string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
            if (string.IsNullOrEmpty(packagePath) || !Directory.Exists(packagePath))
            {
                System.Diagnostics.Debug.WriteLine($"ResourceListViewModel: Package directory not found");
                return;
            }

            var manifestPath = Path.Combine(packagePath, "resource_manifest.json");
            if (!File.Exists(manifestPath))
            {
                System.Diagnostics.Debug.WriteLine($"ResourceListViewModel: Manifest not found at: {manifestPath}");
                return;
            }

            try
            {
                var manifest = JsonConvert.DeserializeObject<Models.ResourceManifest>(File.ReadAllText(manifestPath));
                if (manifest?.Resources != null)
                {
                    foreach (var resource in manifest.Resources)
                    {
                        Resources.Add(new ResourceItem
                        {
                            Id = resource.Id,
                            Name = resource.VirtualPath ?? resource.Path,
                            Path = resource.Path,
                            Type = resource.Type,
                            VirtualPath = resource.VirtualPath
                        });
                    }
                }
            }
            catch
            {
                // Skip invalid manifest
            }
        }

        private void OnResourceSelected(ResourceItem? resource)
        {
            if (resource != null)
            {
                // Notify property panel about resource selection
            }
        }
    }

    public class ResourceItem
    {
        public int Id { get; set; }
        public string Name { get; set; } = string.Empty;
        public string Path { get; set; } = string.Empty;
        public string Type { get; set; } = string.Empty;
        public string? VirtualPath { get; set; }
    }
}
