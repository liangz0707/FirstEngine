using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using FirstEngineEditor.Models;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;
using FirstEngineEditor.Attributes;
using Newtonsoft.Json;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class SceneListViewModel : ViewModelBase
    {
        private readonly IProjectManager _projectManager;
        private readonly IFileManager _fileManager;
        private readonly ISceneManagementService _sceneService;
        private SceneItem? _selectedScene;
        private string? _currentLoadedScenePath;
        private bool _isLoadingScene;

        public SceneListViewModel()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
            _fileManager = ServiceLocator.Get<IFileManager>();
            _sceneService = ServiceLocator.Get<ISceneManagementService>();

            Scenes = new ObservableCollection<SceneItem>();
            
            _projectManager.ProjectChanged += OnProjectChanged;
            _sceneService.SceneLoaded += OnSceneLoaded;
            _sceneService.SceneUnloaded += OnSceneUnloaded;
            
            // Initialize current scene path
            _currentLoadedScenePath = _sceneService.GetCurrentScenePath();
            
            RefreshScenes();
            
            // Commands
            LoadSceneCommand = new RelayCommand(async () => await LoadSceneAsync(), () => SelectedScene != null && !_isLoadingScene);
            ReloadSceneCommand = new RelayCommand(async () => await ReloadSceneAsync(), () => !string.IsNullOrEmpty(_currentLoadedScenePath) && !_isLoadingScene);
            UnloadSceneCommand = new RelayCommand(async () => await UnloadSceneAsync(), () => !string.IsNullOrEmpty(_currentLoadedScenePath) && !_isLoadingScene);
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
            
            // Update command states
            ((RelayCommand)LoadSceneCommand).RaiseCanExecuteChanged();
        }

        private void OnSceneLoaded(object? sender, SceneLoadedEventArgs e)
        {
            _currentLoadedScenePath = e.ScenePath;
            _isLoadingScene = false;
            OnPropertyChanged(nameof(CurrentLoadedSceneName));
            ((RelayCommand)LoadSceneCommand).RaiseCanExecuteChanged();
            ((RelayCommand)ReloadSceneCommand).RaiseCanExecuteChanged();
            ((RelayCommand)UnloadSceneCommand).RaiseCanExecuteChanged();
        }

        private void OnSceneUnloaded(object? sender, EventArgs e)
        {
            _currentLoadedScenePath = null;
            _isLoadingScene = false;
            OnPropertyChanged(nameof(CurrentLoadedSceneName));
            ((RelayCommand)LoadSceneCommand).RaiseCanExecuteChanged();
            ((RelayCommand)ReloadSceneCommand).RaiseCanExecuteChanged();
            ((RelayCommand)UnloadSceneCommand).RaiseCanExecuteChanged();
        }

        public string? CurrentLoadedSceneName
        {
            get
            {
                if (string.IsNullOrEmpty(_currentLoadedScenePath))
                    return null;
                
                var scene = Scenes.FirstOrDefault(s => s.FilePath == _currentLoadedScenePath);
                return scene?.Name ?? Path.GetFileNameWithoutExtension(_currentLoadedScenePath);
            }
        }

        public ICommand LoadSceneCommand { get; }
        public ICommand ReloadSceneCommand { get; }
        public ICommand UnloadSceneCommand { get; }

        public async Task LoadSceneAsync()
        {
            if (SelectedScene == null || _isLoadingScene)
                return;

            try
            {
                _isLoadingScene = true;
                ((RelayCommand)LoadSceneCommand).RaiseCanExecuteChanged();
                
                bool success = await _sceneService.LoadSceneAsync(SelectedScene.FilePath);
                if (success)
                {
                    System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Successfully loaded scene: {SelectedScene.Name}");
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Failed to load scene: {SelectedScene.Name}");
                }
            }
            catch (System.Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Error loading scene: {ex.Message}");
            }
            finally
            {
                _isLoadingScene = false;
                ((RelayCommand)LoadSceneCommand).RaiseCanExecuteChanged();
            }
        }

        public async Task ReloadSceneAsync()
        {
            if (string.IsNullOrEmpty(_currentLoadedScenePath) || _isLoadingScene)
                return;

            try
            {
                _isLoadingScene = true;
                ((RelayCommand)ReloadSceneCommand).RaiseCanExecuteChanged();
                
                // Unload current scene first
                await _sceneService.UnloadSceneAsync();
                
                // Reload the scene
                bool success = await _sceneService.LoadSceneAsync(_currentLoadedScenePath);
                if (success)
                {
                    System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Successfully reloaded scene: {_currentLoadedScenePath}");
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Failed to reload scene: {_currentLoadedScenePath}");
                }
            }
            catch (System.Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Error reloading scene: {ex.Message}");
            }
            finally
            {
                _isLoadingScene = false;
                ((RelayCommand)ReloadSceneCommand).RaiseCanExecuteChanged();
            }
        }

        public async Task UnloadSceneAsync()
        {
            if (string.IsNullOrEmpty(_currentLoadedScenePath) || _isLoadingScene)
                return;

            try
            {
                _isLoadingScene = true;
                ((RelayCommand)UnloadSceneCommand).RaiseCanExecuteChanged();
                
                await _sceneService.UnloadSceneAsync();
                System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Successfully unloaded scene");
            }
            catch (System.Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"SceneListViewModel: Error unloading scene: {ex.Message}");
            }
            finally
            {
                _isLoadingScene = false;
                ((RelayCommand)UnloadSceneCommand).RaiseCanExecuteChanged();
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
        [FirstEngineEditor.Attributes.Property(DisplayName = "ID", Order = 1, Editable = false, Category = "Basic")]
        public int Id { get; set; }

        [JsonProperty("name")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Name", Order = 2, Category = "Basic")]
        public string? Name { get; set; }

        [JsonProperty("active")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Active", Order = 3, Category = "Basic")]
        public bool Active { get; set; }

        [JsonProperty("parentId")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Parent ID", Order = 4, Category = "Basic")]
        public int? ParentId { get; set; }

        [JsonProperty("transform")]
        [FirstEngineEditor.Attributes.PropertyIgnore]
        public TransformData? Transform { get; set; }

        [JsonProperty("components")]
        [FirstEngineEditor.Attributes.PropertyIgnore]
        public List<ComponentData>? Components { get; set; }
    }

    public class TransformData
    {
        [JsonProperty("position")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Position", Order = 1, Category = "Transform", Description = "Entity position in world space")]
        public float[]? Position { get; set; }

        [JsonProperty("rotation")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Rotation", Order = 2, Category = "Transform", Description = "Entity rotation (Euler angles)")]
        public float[]? Rotation { get; set; }

        [JsonProperty("scale")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Scale", Order = 3, Category = "Transform", Description = "Entity scale")]
        public float[]? Scale { get; set; }
    }

    public class ComponentData
    {
        [JsonProperty("type")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Type", Order = 1, Editable = false, Category = "Component")]
        public int Type { get; set; }

        [JsonProperty("modelID")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Model ID", Order = 2, Category = "Model Component", Description = "Model resource ID for Model component")]
        public int? ModelID { get; set; }

        // Camera Component properties
        [JsonProperty("fov")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "FOV", Order = 10, Category = "Camera Component", Description = "Field of view in degrees")]
        public float? FOV { get; set; }

        [JsonProperty("near")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Near Plane", Order = 11, Category = "Camera Component", Description = "Near clipping plane distance")]
        public float? Near { get; set; }

        [JsonProperty("far")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Far Plane", Order = 12, Category = "Camera Component", Description = "Far clipping plane distance")]
        public float? Far { get; set; }

        [JsonProperty("isMainCamera")]
        [FirstEngineEditor.Attributes.Property(DisplayName = "Is Main Camera", Order = 13, Category = "Camera Component", Description = "Whether this is the main camera")]
        public bool IsMainCamera { get; set; }
    }
}
