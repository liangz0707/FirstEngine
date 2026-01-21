using System.Collections.ObjectModel;
using System.IO;
using FirstEngineEditor.Models;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;
using Newtonsoft.Json;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class SceneListViewModel : ViewModelBase
    {
        private readonly IProjectManager _projectManager;
        private readonly IFileManager _fileManager;
        private SceneItem? _selectedScene;

        public SceneListViewModel()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
            _fileManager = ServiceLocator.Get<IFileManager>();

            Scenes = new ObservableCollection<SceneItem>();
            
            _projectManager.ProjectChanged += OnProjectChanged;
            RefreshScenes();
        }

        public ObservableCollection<SceneItem> Scenes { get; }

        public SceneItem? SelectedScene
        {
            get => _selectedScene;
            set
            {
                SetProperty(ref _selectedScene, value);
                OnSceneSelected(value);
            }
        }

        private void OnProjectChanged(object? sender, System.EventArgs e)
        {
            RefreshScenes();
        }

        public void RefreshScenes()
        {
            Scenes.Clear();

            var project = _projectManager.CurrentProject;
            if (project == null) return;

            // 使用统一的路径解析方法获取Package路径
            string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
            if (string.IsNullOrEmpty(packagePath) || !Directory.Exists(packagePath))
            {
                System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Package directory not found");
                return;
            }

            var scenesPath = Path.Combine(packagePath, "Scenes");
            if (!Directory.Exists(scenesPath))
            {
                System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Scenes directory not found at: {scenesPath}");
                return;
            }

            var sceneFiles = Directory.GetFiles(scenesPath, "*.json", SearchOption.TopDirectoryOnly);
            foreach (var file in sceneFiles)
            {
                try
                {
                    var sceneData = JsonConvert.DeserializeObject<SceneData>(File.ReadAllText(file));
                    if (sceneData != null)
                    {
                        Scenes.Add(new SceneItem
                        {
                            Name = sceneData.Name ?? Path.GetFileNameWithoutExtension(file),
                            FilePath = file,
                            SceneData = sceneData
                        });
                    }
                }
                catch
                {
                    // Skip invalid scene files
                }
            }
        }

        private void OnSceneSelected(SceneItem? scene)
        {
            if (scene != null)
            {
                // Notify other panels about scene selection
                // This will be handled by the main view model
            }
        }
    }

    public class SceneItem
    {
        public string Name { get; set; } = string.Empty;
        public string FilePath { get; set; } = string.Empty;
        public SceneData? SceneData { get; set; }
    }

    public class SceneData
    {
        [JsonProperty("version")]
        public int Version { get; set; }

        [JsonProperty("name")]
        public string? Name { get; set; }

        [JsonProperty("levels")]
        public List<LevelData>? Levels { get; set; }
    }

    public class LevelData
    {
        [JsonProperty("name")]
        public string? Name { get; set; }

        [JsonProperty("order")]
        public int Order { get; set; }

        [JsonProperty("visible")]
        public bool Visible { get; set; }

        [JsonProperty("enabled")]
        public bool Enabled { get; set; }

        [JsonProperty("entities")]
        public List<EntityData>? Entities { get; set; }
    }

    public class EntityData
    {
        [JsonProperty("id")]
        public int Id { get; set; }

        [JsonProperty("name")]
        public string? Name { get; set; }

        [JsonProperty("active")]
        public bool Active { get; set; }

        [JsonProperty("transform")]
        public TransformData? Transform { get; set; }

        [JsonProperty("components")]
        public List<ComponentData>? Components { get; set; }
    }

    public class TransformData
    {
        [JsonProperty("position")]
        public float[]? Position { get; set; }

        [JsonProperty("rotation")]
        public float[]? Rotation { get; set; }

        [JsonProperty("scale")]
        public float[]? Scale { get; set; }
    }

    public class ComponentData
    {
        [JsonProperty("type")]
        public int Type { get; set; }

        [JsonProperty("modelID")]
        public int? ModelID { get; set; }
    }
}
