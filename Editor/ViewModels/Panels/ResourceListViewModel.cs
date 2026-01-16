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

            var manifestPath = Path.Combine(project.ProjectPath, "resource_manifest.json");
            if (!File.Exists(manifestPath)) return;

            try
            {
                var manifest = JsonConvert.DeserializeObject<ResourceManifest>(File.ReadAllText(manifestPath));
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

    public class ResourceManifest
    {
        [JsonProperty("version")]
        public int Version { get; set; }

        [JsonProperty("nextID")]
        public int NextID { get; set; }

        [JsonProperty("resources")]
        public List<ResourceEntry>? Resources { get; set; }
    }

    public class ResourceEntry
    {
        [JsonProperty("id")]
        public int Id { get; set; }

        [JsonProperty("path")]
        public string Path { get; set; } = string.Empty;

        [JsonProperty("virtualPath")]
        public string? VirtualPath { get; set; }

        [JsonProperty("type")]
        public string Type { get; set; } = string.Empty;
    }
}
