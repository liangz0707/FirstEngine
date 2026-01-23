using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows.Threading;
using FirstEngineEditor.Models;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;
using FirstEngineEditor.ViewModels.Panels;
using Newtonsoft.Json;

namespace FirstEngineEditor.ViewModels.Panels
{
    /// <summary>
    /// 场景资源列表视图模型 - 显示已加载场景中的资源结构，支持虚拟路径管理和层级操作
    /// </summary>
    public class SceneResourceListViewModel : ViewModelBase
    {
        private readonly ISceneManagementService _sceneManagementService;
        private readonly IProjectManager _projectManager;
        private SceneResourceItem? _selectedItem;
        private string? _currentScenePath;
        private SceneData? _currentSceneData;
        private int _nextEntityId = 1;

        public SceneResourceListViewModel()
        {
            _sceneManagementService = ServiceLocator.Get<ISceneManagementService>();
            _projectManager = ServiceLocator.Get<IProjectManager>();

            SceneResources = new ObservableCollection<SceneResourceItem>();

            _sceneManagementService.SceneLoaded += OnSceneLoaded;
            _sceneManagementService.SceneUnloaded += OnSceneUnloaded;
        }

        public ObservableCollection<SceneResourceItem> SceneResources { get; }

        public SceneResourceItem? SelectedItem
        {
            get => _selectedItem;
            set => SetProperty(ref _selectedItem, value);
        }

        public void SelectItem(SceneResourceItem? item)
        {
            SelectedItem = item;
            
            // Notify property panel about selection change
            // Get PropertyPanelViewModel from MainViewModel via ServiceLocator
            try
            {
                // Try to get MainViewModel first
                var mainViewModel = System.Windows.Application.Current?.MainWindow?.DataContext as MainViewModel;
                if (mainViewModel?.PropertyPanelViewModel != null)
                {
                    var propertyPanel = mainViewModel.PropertyPanelViewModel;
                    
                    // Ensure we're on UI thread
                    var dispatcher = System.Windows.Application.Current?.Dispatcher;
                    if (dispatcher != null && !dispatcher.CheckAccess())
                    {
                        dispatcher.Invoke(() =>
                        {
                            UpdatePropertyPanel(propertyPanel, item);
                        });
                    }
                    else
                    {
                        UpdatePropertyPanel(propertyPanel, item);
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error updating property panel: {ex.Message}");
            }
        }

        private void UpdatePropertyPanel(PropertyPanelViewModel propertyPanel, SceneResourceItem? item)
        {
            if (item != null && item.Type == SceneResourceType.Entity && item.EntityData != null)
            {
                propertyPanel.SelectedObject = item.EntityData;
            }
            else
            {
                propertyPanel.SelectedObject = null;
            }
        }

        public string? CurrentScenePath
        {
            get => _currentScenePath;
            private set => SetProperty(ref _currentScenePath, value);
        }

        private void OnSceneLoaded(object? sender, SceneLoadedEventArgs e)
        {
            // Ensure we're on UI thread
            var dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher != null && !dispatcher.CheckAccess())
            {
                dispatcher.Invoke(() =>
                {
                    CurrentScenePath = e.ScenePath;
                    RefreshSceneResources();
                });
            }
            else
            {
                CurrentScenePath = e.ScenePath;
                RefreshSceneResources();
            }
        }

        private void OnSceneUnloaded(object? sender, EventArgs e)
        {
            // Ensure we're on UI thread
            var dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher != null && !dispatcher.CheckAccess())
            {
                dispatcher.Invoke(() =>
                {
                    CurrentScenePath = null;
                    _currentSceneData = null;
                    SceneResources.Clear();
                });
            }
            else
            {
                CurrentScenePath = null;
                _currentSceneData = null;
                SceneResources.Clear();
            }
        }

        public void RefreshSceneResources()
        {
            SceneResources.Clear();

            if (string.IsNullOrEmpty(_currentScenePath) || !File.Exists(_currentScenePath))
                return;

            try
            {
                // Load scene JSON file
                string jsonContent = File.ReadAllText(_currentScenePath);
                _currentSceneData = JsonConvert.DeserializeObject<SceneData>(jsonContent);
                
                if (_currentSceneData == null)
                    return;

                // Update next entity ID
                UpdateNextEntityId();

                // Create scene item
                var sceneItem = new SceneResourceItem
                {
                    Name = _currentSceneData.Name ?? Path.GetFileNameWithoutExtension(_currentScenePath),
                    Type = SceneResourceType.Scene,
                    VirtualPath = _currentSceneData.Name ?? "Scene",
                    FilePath = _currentScenePath,
                    SceneData = _currentSceneData,
                    Children = new ObservableCollection<SceneResourceItem>()
                };

                // Add scene levels
                if (_currentSceneData.Levels != null)
                {
                    foreach (var levelData in _currentSceneData.Levels.OrderBy(l => l.Order))
                    {
                        var levelItem = CreateLevelItem(levelData, sceneItem);
                        sceneItem.Children.Add(levelItem);
                    }
                }

                SceneResources.Add(sceneItem);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error refreshing scene resources: {ex.Message}");
            }
        }

        private SceneResourceItem CreateLevelItem(LevelData levelData, SceneResourceItem parent)
        {
            var levelItem = new SceneResourceItem
            {
                Name = levelData.Name ?? "Unnamed Level",
                Type = SceneResourceType.SceneLevel,
                VirtualPath = $"{parent.VirtualPath}/{levelData.Name ?? "Level"}",
                Parent = parent,
                LevelData = levelData,
                Children = new ObservableCollection<SceneResourceItem>()
            };

            // Add entities as resources (build hierarchy)
            if (levelData.Entities != null)
            {
                // First, create all entity items
                var entityItems = new Dictionary<int, SceneResourceItem>();
                foreach (var entityData in levelData.Entities)
                {
                    var entityItem = new SceneResourceItem
                    {
                        Name = entityData.Name ?? $"Entity_{entityData.Id}",
                        Type = SceneResourceType.Entity,
                        VirtualPath = $"{levelItem.VirtualPath}/{entityData.Name ?? $"Entity_{entityData.Id}"}",
                        Parent = levelItem,
                        EntityData = entityData,
                        Children = new ObservableCollection<SceneResourceItem>()
                    };

                    // Note: Components are no longer displayed in the scene browser tree
                    // They are only shown in the Entity property panel

                    entityItems[entityData.Id] = entityItem;
                }

                // Build hierarchy based on ParentId
                foreach (var kvp in entityItems)
                {
                    var entityItem = kvp.Value;
                    var entityData = entityItem.EntityData;
                    
                    if (entityData != null && entityData.ParentId.HasValue)
                    {
                        // This entity has a parent
                        if (entityItems.TryGetValue(entityData.ParentId.Value, out var parentEntity))
                        {
                            // Move to parent's children
                            entityItem.Parent = parentEntity;
                            entityItem.VirtualPath = $"{parentEntity.VirtualPath}/{entityData.Name ?? $"Entity_{entityData.Id}"}";
                            if (parentEntity.Children == null)
                                parentEntity.Children = new ObservableCollection<SceneResourceItem>();
                            parentEntity.Children.Add(entityItem);
                        }
                        else
                        {
                            // Parent not found, add to level
                            levelItem.Children.Add(entityItem);
                        }
                    }
                    else
                    {
                        // Root entity, add to level
                        levelItem.Children.Add(entityItem);
                    }
                }
            }

            return levelItem;
        }

        private void UpdateNextEntityId()
        {
            if (_currentSceneData?.Levels == null)
            {
                _nextEntityId = 1;
                return;
            }

            int maxId = 0;
            foreach (var level in _currentSceneData.Levels)
            {
                if (level.Entities != null)
                {
                    foreach (var entity in level.Entities)
                    {
                        if (entity.Id > maxId)
                            maxId = entity.Id;
                    }
                }
            }
            _nextEntityId = maxId + 1;
        }

        // ========== SceneLevel Operations ==========

        public bool CanAddSceneLevel(SceneResourceItem? parent)
        {
            return parent != null && parent.Type == SceneResourceType.Scene;
        }

        public void AddSceneLevel(SceneResourceItem? parent, string? name = null)
        {
            if (!CanAddSceneLevel(parent) || _currentSceneData == null)
                return;

            if (string.IsNullOrEmpty(name))
                name = $"Level_{DateTime.Now:yyyyMMdd_HHmmss}";

            var newLevel = new LevelData
            {
                Name = name,
                Order = _currentSceneData.Levels?.Count ?? 0,
                Visible = true,
                Enabled = true,
                Entities = new List<EntityData>()
            };

            if (_currentSceneData.Levels == null)
                _currentSceneData.Levels = new List<LevelData>();
            
            _currentSceneData.Levels.Add(newLevel);

            // Update UI
            var levelItem = CreateLevelItem(newLevel, parent!);
            parent!.Children.Add(levelItem);

            SaveSceneData();
        }

        public bool CanDeleteSceneLevel(SceneResourceItem? item)
        {
            return item != null && item.Type == SceneResourceType.SceneLevel;
        }

        public void DeleteSceneLevel(SceneResourceItem item)
        {
            if (!CanDeleteSceneLevel(item) || _currentSceneData == null || item.Parent == null)
                return;

            // Remove from data
            if (_currentSceneData.Levels != null && item.LevelData != null)
            {
                _currentSceneData.Levels.Remove(item.LevelData);
            }

            // Remove from UI
            item.Parent.Children?.Remove(item);

            SaveSceneData();
        }

        public bool CanMoveSceneLevel(SceneResourceItem? item, SceneResourceItem? newParent)
        {
            return item != null && item.Type == SceneResourceType.SceneLevel &&
                   newParent != null && newParent.Type == SceneResourceType.Scene;
        }

        public void MoveSceneLevel(SceneResourceItem item, SceneResourceItem newParent)
        {
            if (!CanMoveSceneLevel(item, newParent) || item.Parent == null)
                return;

            // Remove from old parent
            item.Parent.Children?.Remove(item);

            // Update parent and virtual path
            item.Parent = newParent;
            item.VirtualPath = $"{newParent.VirtualPath}/{item.Name}";

            // Add to new parent
            if (newParent.Children == null)
                newParent.Children = new ObservableCollection<SceneResourceItem>();
            newParent.Children.Add(item);

            SaveSceneData();
        }

        // ========== Entity Operations ==========

        public bool CanAddEntity(SceneResourceItem? parent)
        {
            return parent != null && (parent.Type == SceneResourceType.SceneLevel || parent.Type == SceneResourceType.Entity);
        }

        public void AddEntity(SceneResourceItem? parent, string? name = null, int? parentEntityId = null)
        {
            if (!CanAddEntity(parent) || _currentSceneData == null)
            {
                System.Diagnostics.Debug.WriteLine($"AddEntity: Cannot add entity - CanAddEntity={CanAddEntity(parent)}, _currentSceneData={_currentSceneData != null}");
                return;
            }

            if (parent == null)
            {
                System.Diagnostics.Debug.WriteLine("AddEntity: parent is null");
                return;
            }

            // Find the level that contains this parent
            LevelData? targetLevel = null;
            if (parent.Type == SceneResourceType.SceneLevel)
            {
                targetLevel = parent.LevelData;
                if (targetLevel == null)
                {
                    System.Diagnostics.Debug.WriteLine($"AddEntity: parent.LevelData is null for SceneLevel '{parent.Name}'");
                    return;
                }
            }
            else if (parent.Type == SceneResourceType.Entity && parent.EntityData != null)
            {
                // Find the level containing this entity
                foreach (var level in _currentSceneData.Levels ?? Enumerable.Empty<LevelData>())
                {
                    if (level.Entities != null && level.Entities.Any(e => e.Id == parent.EntityData.Id))
                    {
                        targetLevel = level;
                        parentEntityId = parent.EntityData.Id;
                        break;
                    }
                }
                
                if (targetLevel == null)
                {
                    System.Diagnostics.Debug.WriteLine($"AddEntity: Could not find level containing entity ID={parent.EntityData.Id}");
                    return;
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"AddEntity: Unexpected parent type: {parent.Type}");
                return;
            }

            if (targetLevel == null)
            {
                System.Diagnostics.Debug.WriteLine("AddEntity: targetLevel is null after search");
                return;
            }

            if (string.IsNullOrEmpty(name))
                name = $"Entity_{_nextEntityId}";

            try
            {
                var newEntity = new EntityData
                {
                    Id = _nextEntityId++,
                    Name = name,
                    Active = true,
                    ParentId = parentEntityId,
                    Transform = new TransformData
                    {
                        Position = new float[] { 0, 0, 0 },
                        Rotation = new float[] { 0, 0, 0 },
                        Scale = new float[] { 1, 1, 1 }
                    },
                    Components = new List<ComponentData>()
                };

                if (targetLevel.Entities == null)
                    targetLevel.Entities = new List<EntityData>();
                
                targetLevel.Entities.Add(newEntity);

                // Ensure the entity is properly saved
                System.Diagnostics.Debug.WriteLine($"AddEntity: Created entity ID={newEntity.Id}, Name={newEntity.Name}, ParentId={parentEntityId}, Level='{targetLevel.Name}'");

                // Update UI - ensure we're on UI thread
                var dispatcher = System.Windows.Application.Current?.Dispatcher;
                if (dispatcher != null && !dispatcher.CheckAccess())
                {
                    dispatcher.Invoke(() =>
                    {
                        AddEntityToUI(parent, name, newEntity, parentEntityId);
                    });
                }
                else
                {
                    AddEntityToUI(parent, name, newEntity, parentEntityId);
                }

                SaveSceneData();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in AddEntity: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error creating entity: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void AddEntityToUI(SceneResourceItem parent, string name, EntityData newEntity, int? parentEntityId)
        {
            try
            {
                if (parent == null)
                {
                    System.Diagnostics.Debug.WriteLine("AddEntityToUI: parent is null");
                    return;
                }

                // Update UI
                var entityItem = new SceneResourceItem
                {
                    Name = name,
                    Type = SceneResourceType.Entity,
                    VirtualPath = parentEntityId.HasValue 
                        ? $"{parent.VirtualPath}/{name}"
                        : $"{parent.VirtualPath}/{name}",
                    Parent = parent,
                    EntityData = newEntity,
                    Children = new ObservableCollection<SceneResourceItem>()
                };

                if (parent.Children == null)
                    parent.Children = new ObservableCollection<SceneResourceItem>();
                
                parent.Children.Add(entityItem);

                System.Diagnostics.Debug.WriteLine($"AddEntityToUI: Added entity item to parent '{parent.Name}', parent.Children.Count={parent.Children.Count}");
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error adding entity to UI: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error adding entity to UI: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        public bool CanDeleteEntity(SceneResourceItem? item)
        {
            return item != null && item.Type == SceneResourceType.Entity;
        }

        public void DeleteEntity(SceneResourceItem item)
        {
            if (!CanDeleteEntity(item) || _currentSceneData == null || item.Parent == null || item.EntityData == null)
                return;

            // Find and remove from level data
            foreach (var level in _currentSceneData.Levels ?? Enumerable.Empty<LevelData>())
            {
                if (level.Entities != null && level.Entities.Remove(item.EntityData))
                {
                    // Also remove all child entities
                    RemoveChildEntities(level.Entities, item.EntityData.Id);
                    break;
                }
            }

            // Remove from UI
            item.Parent.Children?.Remove(item);

            SaveSceneData();
        }

        private void RemoveChildEntities(List<EntityData> entities, int parentId)
        {
            var children = entities.Where(e => e.ParentId == parentId).ToList();
            foreach (var child in children)
            {
                RemoveChildEntities(entities, child.Id);
                entities.Remove(child);
            }
        }

        public bool CanMoveEntity(SceneResourceItem? item, SceneResourceItem? newParent)
        {
            return item != null && item.Type == SceneResourceType.Entity &&
                   newParent != null && (newParent.Type == SceneResourceType.SceneLevel || newParent.Type == SceneResourceType.Entity) &&
                   item != newParent; // Cannot move to itself
        }

        public void MoveEntity(SceneResourceItem item, SceneResourceItem newParent)
        {
            if (!CanMoveEntity(item, newParent) || item.Parent == null || item.EntityData == null)
                return;

            // Update entity's parent ID
            if (newParent.Type == SceneResourceType.Entity && newParent.EntityData != null)
            {
                item.EntityData.ParentId = newParent.EntityData.Id;
            }
            else
            {
                item.EntityData.ParentId = null;
            }

            // Remove from old parent
            item.Parent.Children?.Remove(item);

            // Update parent and virtual path
            item.Parent = newParent;
            item.VirtualPath = $"{newParent.VirtualPath}/{item.Name}";

            // Update all children's virtual paths recursively
            UpdateEntityVirtualPaths(item);

            // Add to new parent
            if (newParent.Children == null)
                newParent.Children = new ObservableCollection<SceneResourceItem>();
            newParent.Children.Add(item);

            SaveSceneData();
        }

        private void UpdateEntityVirtualPaths(SceneResourceItem entityItem)
        {
            if (entityItem.Children == null)
                return;

            foreach (var child in entityItem.Children)
            {
                if (child.Type == SceneResourceType.Entity)
                {
                    child.VirtualPath = $"{entityItem.VirtualPath}/{child.Name}";
                    UpdateEntityVirtualPaths(child);
                }
            }
        }

        // ========== Resource Operations ==========

        public bool CanAddResource(SceneResourceItem? parent, int modelId)
        {
            return parent != null && parent.Type == SceneResourceType.Entity && modelId > 0;
        }

        public void AddResourceToEntity(SceneResourceItem entityItem, int modelId)
        {
            if (!CanAddResource(entityItem, modelId) || entityItem.EntityData == null)
                return;

            // Add component to entity
            if (entityItem.EntityData.Components == null)
                entityItem.EntityData.Components = new List<ComponentData>();

            var component = new ComponentData
            {
                Type = 1, // Model component type
                ModelID = modelId
            };

            entityItem.EntityData.Components.Add(component);

            // Update UI
            var resourceItem = new SceneResourceItem
            {
                Name = $"Model_{modelId}",
                Type = SceneResourceType.Resource,
                VirtualPath = $"{entityItem.VirtualPath}/Model_{modelId}",
                Parent = entityItem,
                ResourceId = modelId
            };

            if (entityItem.Children == null)
                entityItem.Children = new ObservableCollection<SceneResourceItem>();
            entityItem.Children.Add(resourceItem);

            SaveSceneData();
        }

        public bool CanDeleteResource(SceneResourceItem? item)
        {
            return item != null && item.Type == SceneResourceType.Resource;
        }

        public void DeleteResource(SceneResourceItem item)
        {
            if (!CanDeleteResource(item) || item.Parent == null || item.Parent.EntityData == null || !item.ResourceId.HasValue)
                return;

            // Remove component from entity
            if (item.Parent.EntityData.Components != null)
            {
                var component = item.Parent.EntityData.Components.FirstOrDefault(c => c.ModelID == item.ResourceId.Value);
                if (component != null)
                {
                    item.Parent.EntityData.Components.Remove(component);
                }
            }

            // Remove from UI
            item.Parent.Children?.Remove(item);

            SaveSceneData();
        }

        // ========== Save Scene Data ==========

        public void SaveSceneData()
        {
            if (_currentSceneData == null || string.IsNullOrEmpty(_currentScenePath))
            {
                System.Diagnostics.Debug.WriteLine($"SaveSceneData: Cannot save - _currentSceneData is null or path is empty");
                return;
            }

            try
            {
                // Ensure we're on UI thread for file operations (though File.WriteAllText should be thread-safe)
                var dispatcher = System.Windows.Application.Current?.Dispatcher;
                if (dispatcher != null && !dispatcher.CheckAccess())
                {
                    dispatcher.Invoke(() =>
                    {
                        SaveSceneDataInternal();
                    });
                }
                else
                {
                    SaveSceneDataInternal();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error saving scene data: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                throw; // Re-throw to let caller handle
            }
        }

        private void SaveSceneDataInternal()
        {
            try
            {
                if (_currentSceneData == null || string.IsNullOrEmpty(_currentScenePath))
                {
                    System.Diagnostics.Debug.WriteLine($"SaveSceneDataInternal: Cannot save - _currentSceneData is null or path is empty");
                    return;
                }

                // Normalize path to absolute path
                string normalizedPath = Path.GetFullPath(_currentScenePath);
                
                // Serialize scene data to JSON with settings to handle potential circular references
                var settings = new JsonSerializerSettings
                {
                    Formatting = Formatting.Indented,
                    NullValueHandling = NullValueHandling.Ignore,
                    ReferenceLoopHandling = ReferenceLoopHandling.Ignore
                };
                string json = JsonConvert.SerializeObject(_currentSceneData, settings);
                
                // Ensure directory exists
                string? directory = Path.GetDirectoryName(normalizedPath);
                if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                    System.Diagnostics.Debug.WriteLine($"SaveSceneDataInternal: Created directory: {directory}");
                }
                
                // Write JSON to file
                File.WriteAllText(normalizedPath, json);
                System.Diagnostics.Debug.WriteLine($"SceneResourceListViewModel: Saved scene data to {normalizedPath}");
                System.Diagnostics.Debug.WriteLine($"SaveSceneDataInternal: Scene has {_currentSceneData.Levels?.Count ?? 0} levels");
                
                // Log entity count for debugging
                if (_currentSceneData.Levels != null)
                {
                    foreach (var level in _currentSceneData.Levels)
                    {
                        System.Diagnostics.Debug.WriteLine($"SaveSceneDataInternal: Level '{level.Name}' has {level.Entities?.Count ?? 0} entities");
                        if (level.Entities != null)
                        {
                            foreach (var entity in level.Entities)
                            {
                                System.Diagnostics.Debug.WriteLine($"SaveSceneDataInternal:   - Entity ID={entity.Id}, Name={entity.Name}, ParentId={entity.ParentId}");
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in SaveSceneDataInternal: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                throw; // Re-throw to let SaveSceneData handle
            }
        }

        // ========== Rename Operations ==========

        public bool CanRename(SceneResourceItem? item)
        {
            return item != null && (item.Type == SceneResourceType.SceneLevel || 
                                    item.Type == SceneResourceType.Entity);
        }

        public void RenameItem(SceneResourceItem item, string newName)
        {
            if (!CanRename(item) || string.IsNullOrEmpty(newName))
                return;

            item.Name = newName;
            item.VirtualPath = item.Parent != null 
                ? $"{item.Parent.VirtualPath}/{newName}"
                : newName;

            // Update data
            if (item.Type == SceneResourceType.SceneLevel && item.LevelData != null)
            {
                item.LevelData.Name = newName;
            }
            else if (item.Type == SceneResourceType.Entity && item.EntityData != null)
            {
                item.EntityData.Name = newName;
            }

            // Update children virtual paths
            if (item.Type == SceneResourceType.Entity)
            {
                UpdateEntityVirtualPaths(item);
            }

            SaveSceneData();
        }
    }

    public enum SceneResourceType
    {
        Scene,
        SceneLevel,
        Entity,
        Resource
    }

    public class SceneResourceItem
    {
        public string Name { get; set; } = string.Empty;
        public SceneResourceType Type { get; set; }
        public string VirtualPath { get; set; } = string.Empty;
        public string? FilePath { get; set; }
        public SceneResourceItem? Parent { get; set; }
        public ObservableCollection<SceneResourceItem>? Children { get; set; }
        public SceneData? SceneData { get; set; }
        public LevelData? LevelData { get; set; }
        public EntityData? EntityData { get; set; }
        public int? ResourceId { get; set; }

        public string DisplayIcon
        {
            get
            {
                return Type switch
                {
                    SceneResourceType.Scene => "🌍",
                    SceneResourceType.SceneLevel => "📚",
                    SceneResourceType.Entity => "🎭",
                    SceneResourceType.Resource => "📦",
                    _ => "📄"
                };
            }
        }
    }
}
