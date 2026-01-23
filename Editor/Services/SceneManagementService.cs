using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Windows.Threading;
using FirstEngineEditor.ViewModels.Panels;
using Newtonsoft.Json;

namespace FirstEngineEditor.Services
{
    public class SceneManagementService : ISceneManagementService
    {
        private readonly IProjectManager _projectManager;
        private static RenderEngineService? s_renderEngine;
        private string? _currentScenePath;
        private bool _hasLoadedScene = false;

        public string? CurrentScenePath
        {
            get => _currentScenePath;
            private set
            {
                if (_currentScenePath != value)
                {
                    _currentScenePath = value;
                    if (string.IsNullOrEmpty(value))
                    {
                        _hasLoadedScene = false;
                        SceneUnloaded?.Invoke(this, EventArgs.Empty);
                    }
                }
            }
        }

        public event EventHandler<SceneLoadedEventArgs>? SceneLoaded;
        public event EventHandler? SceneUnloaded;

        public SceneManagementService()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
        }

        public async Task<bool> LoadSceneAsync(string scenePath)
        {
            return await Task.Run(() =>
            {
                try
                {
                    if (string.IsNullOrEmpty(scenePath))
                        return false;
                    
                    // Normalize path to absolute path
                    string normalizedPath = Path.GetFullPath(scenePath);
                    
                    if (!File.Exists(normalizedPath))
                    {
                        System.Diagnostics.Debug.WriteLine($"SceneManagementService: Scene file does not exist: {normalizedPath}");
                        return false;
                    }

                    // Get render engine from static reference
                    if (s_renderEngine == null)
                    {
                        System.Diagnostics.Debug.WriteLine("SceneManagementService: Render engine not available yet");
                        return false;
                    }

                    // If scene is already loaded, unload it first
                    if (_hasLoadedScene && !string.IsNullOrEmpty(_currentScenePath))
                    {
                        s_renderEngine.UnloadScene();
                        _hasLoadedScene = false;
                    }

                    // Load new scene (use normalized path)
                    s_renderEngine.LoadScene(normalizedPath);
                    _currentScenePath = normalizedPath;
                    _hasLoadedScene = true;

                    // Invoke event on UI thread
                    var dispatcher = System.Windows.Application.Current?.Dispatcher;
                    if (dispatcher != null && !dispatcher.CheckAccess())
                    {
                        dispatcher.Invoke(() =>
                        {
                            SceneLoaded?.Invoke(this, new SceneLoadedEventArgs(normalizedPath, true));
                        });
                    }
                    else
                    {
                        SceneLoaded?.Invoke(this, new SceneLoadedEventArgs(normalizedPath, true));
                    }
                    
                    System.Diagnostics.Debug.WriteLine($"SceneManagementService: Loaded scene from {normalizedPath}");
                    return true;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error loading scene: {ex.Message}");
                    return false;
                }
            });
        }

        public async Task<bool> LoadSceneLevelAsync(string levelPath)
        {
            return await Task.Run(() =>
            {
                try
                {
                    if (string.IsNullOrEmpty(levelPath))
                        return false;
                    
                    // Normalize path to absolute path
                    string normalizedLevelPath = Path.GetFullPath(levelPath);
                    
                    if (!File.Exists(normalizedLevelPath))
                    {
                        System.Diagnostics.Debug.WriteLine($"SceneManagementService: Level file does not exist: {normalizedLevelPath}");
                        return false;
                    }

                    // Check if we have a loaded scene
                    if (!_hasLoadedScene || string.IsNullOrEmpty(_currentScenePath) || s_renderEngine == null)
                    {
                        System.Diagnostics.Debug.WriteLine("SceneManagementService: No scene loaded, cannot load level");
                        return false;
                    }

                    // Load level JSON file (use normalized path)
                    string jsonContent = File.ReadAllText(normalizedLevelPath);
                    var levelData = JsonConvert.DeserializeObject<LevelData>(jsonContent);
                    
                    if (levelData == null)
                    {
                        // Try to load as scene file and extract first level
                        var sceneData = JsonConvert.DeserializeObject<SceneData>(jsonContent);
                        if (sceneData?.Levels != null && sceneData.Levels.Count > 0)
                        {
                            levelData = sceneData.Levels[0];
                        }
                    }

                    if (levelData == null)
                    {
                        System.Diagnostics.Debug.WriteLine("SceneManagementService: Failed to parse level file");
                        return false;
                    }

                    // Merge level into current scene by reloading the scene file and adding the level
                    // For now, we'll reload the entire scene with the new level merged
                    // This is a simplified approach - ideally we'd have C++ API to add levels directly
                    
                    // Load current scene data
                    string currentSceneJson = File.ReadAllText(_currentScenePath);
                    var currentSceneData = JsonConvert.DeserializeObject<SceneData>(currentSceneJson);
                    
                    if (currentSceneData != null)
                    {
                        // Add or update level in scene data
                        if (currentSceneData.Levels == null)
                            currentSceneData.Levels = new List<LevelData>();
                        
                        // Check if level already exists
                        var existingLevel = currentSceneData.Levels.FirstOrDefault(l => 
                            l.Name?.Equals(levelData.Name, StringComparison.OrdinalIgnoreCase) == true);
                        
                        if (existingLevel != null)
                        {
                            // Update existing level
                            existingLevel.Entities = levelData.Entities;
                            existingLevel.Order = levelData.Order;
                            existingLevel.Visible = levelData.Visible;
                            existingLevel.Enabled = levelData.Enabled;
                        }
                        else
                        {
                            // Add new level
                            currentSceneData.Levels.Add(levelData);
                        }

                        // Save merged scene to temp file and reload
                        string tempScenePath = Path.Combine(Path.GetDirectoryName(_currentScenePath) ?? "", 
                            Path.GetFileNameWithoutExtension(_currentScenePath) + "_temp.json");
                        
                        File.WriteAllText(tempScenePath, JsonConvert.SerializeObject(currentSceneData, Formatting.Indented));
                        
                        // Reload scene
                        s_renderEngine.UnloadScene();
                        s_renderEngine.LoadScene(tempScenePath);
                        
                        // Replace original file
                        File.Delete(_currentScenePath);
                        File.Move(tempScenePath, _currentScenePath);
                        
                        // Trigger refresh on UI thread
                        var dispatcher = System.Windows.Application.Current?.Dispatcher;
                        if (dispatcher != null && !dispatcher.CheckAccess())
                        {
                            dispatcher.Invoke(() =>
                            {
                                SceneLoaded?.Invoke(this, new SceneLoadedEventArgs(_currentScenePath, false));
                            });
                        }
                        else
                        {
                            SceneLoaded?.Invoke(this, new SceneLoadedEventArgs(_currentScenePath, false));
                        }
                        
                        System.Diagnostics.Debug.WriteLine($"SceneManagementService: Merged level from {levelPath} into scene");
                        return true;
                    }
                    
                    return false;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error loading scene level: {ex.Message}");
                    return false;
                }
            });
        }

        public async Task UnloadSceneAsync()
        {
            await Task.Run(() =>
            {
                try
                {
                    if (s_renderEngine != null && _hasLoadedScene)
                    {
                        s_renderEngine.UnloadScene();
                        _hasLoadedScene = false;
                    }
                    
                    // Invoke event on UI thread
                    var dispatcher = System.Windows.Application.Current?.Dispatcher;
                    if (dispatcher != null && !dispatcher.CheckAccess())
                    {
                        dispatcher.Invoke(() =>
                        {
                            CurrentScenePath = null;
                        });
                    }
                    else
                    {
                        CurrentScenePath = null;
                    }
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error unloading scene: {ex.Message}");
                }
            });
        }

        public async Task<bool> SaveSceneAsync(string? scenePath = null)
        {
            return await Task.Run(() =>
            {
                try
                {
                    if (!_hasLoadedScene || string.IsNullOrEmpty(_currentScenePath) || s_renderEngine == null)
                        return false;

                    string targetPath = scenePath ?? _currentScenePath;
                    if (string.IsNullOrEmpty(targetPath))
                        return false;

                    // Ensure directory exists
                    string? directory = Path.GetDirectoryName(targetPath);
                    if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                    {
                        Directory.CreateDirectory(directory);
                    }

                    // Save scene using render engine
                    bool success = s_renderEngine.SaveScene(targetPath);
                    if (success)
                    {
                        // Update current scene path if saving to a new location
                        if (!string.IsNullOrEmpty(scenePath) && scenePath != _currentScenePath)
                        {
                            _currentScenePath = scenePath;
                        }
                        System.Diagnostics.Debug.WriteLine($"SceneManagementService: Saved scene to {targetPath}");
                    }
                    else
                    {
                        System.Diagnostics.Debug.WriteLine($"SceneManagementService: Failed to save scene to {targetPath}");
                    }
                    
                    return success;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error saving scene: {ex.Message}");
                    return false;
                }
            });
        }

        public string? GetCurrentScenePath()
        {
            return CurrentScenePath;
        }

        /// <summary>
        /// 设置渲染引擎引用（由RenderView调用）
        /// </summary>
        public static void SetRenderEngine(RenderEngineService renderEngine)
        {
            s_renderEngine = renderEngine;
        }
    }
}
