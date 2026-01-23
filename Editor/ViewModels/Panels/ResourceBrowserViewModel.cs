using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using FirstEngineEditor.Models;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;
using Newtonsoft.Json;
using System.Xml.Linq;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class ResourceBrowserViewModel : ViewModelBase
    {
        private readonly IProjectManager _projectManager;
        private readonly IFileManager _fileManager;
        private ResourceBrowserItem? _selectedResource;
        private string _currentFolderPath = "";
        private string _searchText = "";
        private string? _typeFilter;

        public ResourceBrowserViewModel()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
            _fileManager = ServiceLocator.Get<IFileManager>();

            Resources = new ObservableCollection<ResourceBrowserItem>();
            FilteredResources = new ObservableCollection<ResourceBrowserItem>();
            Folders = new ObservableCollection<FolderItem>();

            _projectManager.ProjectChanged += OnProjectChanged;
            RefreshResources();
        }

        public ObservableCollection<ResourceBrowserItem> Resources { get; }
        public ObservableCollection<ResourceBrowserItem> FilteredResources { get; }
        public ObservableCollection<FolderItem> Folders { get; }

        public ResourceBrowserItem? SelectedResource
        {
            get => _selectedResource;
            set
            {
                SetProperty(ref _selectedResource, value);
                OnResourceSelected(value);
            }
        }

        public string CurrentFolderPath
        {
            get => _currentFolderPath;
            private set => SetProperty(ref _currentFolderPath, value);
        }

        public void RefreshResources()
        {
            Resources.Clear();
            FilteredResources.Clear();
            Folders.Clear();

            var project = _projectManager.CurrentProject;
            if (project == null)
            {
                System.Diagnostics.Debug.WriteLine("RefreshResources: No current project");
                Console.WriteLine("RefreshResources: No current project - please open a project first");
                return;
            }

            // 使用统一的路径解析方法
            string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
            System.Diagnostics.Debug.WriteLine($"RefreshResources: Project path: {project.ProjectPath}");
            System.Diagnostics.Debug.WriteLine($"RefreshResources: Resolved Package path: {packagePath ?? "null"}");
            Console.WriteLine($"RefreshResources: Project path: {project.ProjectPath}");
            Console.WriteLine($"RefreshResources: Resolved Package path: {packagePath ?? "null"}");
            
            if (string.IsNullOrEmpty(packagePath) || !Directory.Exists(packagePath))
            {
                System.Diagnostics.Debug.WriteLine($"RefreshResources: Package directory does not exist: {packagePath ?? "null"}");
                Console.WriteLine($"RefreshResources: Package directory does not exist: {packagePath ?? "null"}");
                return;
            }

            // Load resource manifest
            string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
            System.Diagnostics.Debug.WriteLine($"RefreshResources: Loading manifest from: {manifestPath}");
            
            if (File.Exists(manifestPath))
            {
                try
                {
                    string jsonContent = File.ReadAllText(manifestPath);
                    System.Diagnostics.Debug.WriteLine($"RefreshResources: Manifest file size: {jsonContent.Length} bytes");
                    
                    var manifest = JsonConvert.DeserializeObject<ResourceManifest>(jsonContent);
                    if (manifest?.Resources != null)
                    {
                        System.Diagnostics.Debug.WriteLine($"RefreshResources: Found {manifest.Resources.Count} resources in manifest");
                        
                        foreach (var resource in manifest.Resources)
                        {
                            var item = new ResourceBrowserItem
                            {
                                Id = resource.Id,
                                Name = GetResourceDisplayName(resource),
                                VirtualPath = resource.VirtualPath ?? resource.Path,
                                Path = resource.Path,
                                Type = resource.Type,
                                ThumbnailPath = GetThumbnailPath(resource, packagePath)
                            };

                            Resources.Add(item);
                            System.Diagnostics.Debug.WriteLine($"Added resource: {item.Name} (ID={item.Id}, Type={item.Type}, VirtualPath={item.VirtualPath})");
                        }
                        
                        Console.WriteLine($"Loaded {manifest.Resources.Count} resources from manifest");
                    }
                    else
                    {
                        System.Diagnostics.Debug.WriteLine("RefreshResources: Manifest has no resources");
                    }
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error loading manifest: {ex.Message}");
                    System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                    Console.WriteLine($"Error loading manifest: {ex.Message}");
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"RefreshResources: Manifest file does not exist: {manifestPath}");
            }

            // Also scan Scenes directory for scene files not in manifest
            ScanSceneFiles(packagePath);
            
            // Also scan Shaders directory for shader files not in manifest
            ScanShaderFiles(packagePath);

            // Build folder structure
            BuildFolderTree();

            // Apply filters
            ApplyFilters();
            
            System.Diagnostics.Debug.WriteLine($"RefreshResources: Total resources loaded: {Resources.Count}, Filtered: {FilteredResources.Count}");
            Console.WriteLine($"RefreshResources: Total resources loaded: {Resources.Count}, Filtered: {FilteredResources.Count}");
        }

        public void FilterByType(string? type)
        {
            _typeFilter = type;
            ApplyFilters();
        }

        public void SearchResources(string searchText)
        {
            _searchText = searchText ?? "";
            ApplyFilters();
        }

        public void NavigateToFolder(string folderPath)
        {
            CurrentFolderPath = folderPath;
            ApplyFilters();
        }

        public void SelectResource(ResourceBrowserItem? item)
        {
            SelectedResource = item;
        }

        /// <summary>
        /// 只刷新资源列表，不重建文件夹树（用于移动资源等操作）
        /// </summary>
        public void RefreshResourcesOnly()
        {
            var project = _projectManager.CurrentProject;
            if (project == null) return;

            string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
            if (string.IsNullOrEmpty(packagePath) || !Directory.Exists(packagePath))
                return;

            string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
            if (!File.Exists(manifestPath))
                return;

            try
            {
                string jsonContent = File.ReadAllText(manifestPath);
                var manifest = JsonConvert.DeserializeObject<ResourceManifest>(jsonContent);
                
                // Store current selection
                int? selectedId = SelectedResource?.Id;
                
                Resources.Clear();
                FilteredResources.Clear();

                if (manifest?.Resources != null)
                {
                    foreach (var resource in manifest.Resources)
                    {
                        var item = new ResourceBrowserItem
                        {
                            Id = resource.Id,
                            Name = GetResourceDisplayName(resource),
                            VirtualPath = resource.VirtualPath ?? resource.Path,
                            Path = resource.Path,
                            Type = resource.Type,
                            ThumbnailPath = GetThumbnailPath(resource, packagePath)
                        };

                        Resources.Add(item);
                    }
                }

                // Restore selection if possible
                if (selectedId.HasValue)
                {
                    var restoredResource = Resources.FirstOrDefault(r => r.Id == selectedId.Value);
                    if (restoredResource != null)
                    {
                        SelectedResource = restoredResource;
                    }
                }

                // Apply filters
                ApplyFilters();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error refreshing resources: {ex.Message}");
            }
        }

        /// <summary>
        /// 只更新文件夹树结构，不刷新资源列表
        /// </summary>
        public void UpdateFolderTree()
        {
            Folders.Clear();
            BuildFolderTree();
        }

        private void BuildFolderTree()
        {
            var rootFolder = new FolderItem
            {
                Name = "Package",
                Path = "",
                Children = new ObservableCollection<FolderItem>()
            };

            var project = _projectManager.CurrentProject;
            if (project == null)
            {
                Folders.Add(rootFolder);
                return;
            }

            string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
            if (string.IsNullOrEmpty(packagePath) || !Directory.Exists(packagePath))
            {
                Folders.Add(rootFolder);
                return;
            }

            // Load manifest to get empty folders
            string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
            List<string> emptyFolders = new List<string>();
            if (File.Exists(manifestPath))
            {
                try
                {
                    string jsonContent = File.ReadAllText(manifestPath);
                    var manifest = JsonConvert.DeserializeObject<ResourceManifest>(jsonContent);
                    if (manifest?.EmptyFolders != null)
                    {
                        emptyFolders = manifest.EmptyFolders;
                    }
                }
                catch
                {
                    // Ignore errors
                }
            }

            // Build folder structure from virtual paths
            var folderSet = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            
            // Add folders from resources
            foreach (var resource in Resources)
            {
                if (string.IsNullOrEmpty(resource.VirtualPath))
                    continue;

                string[] parts = resource.VirtualPath.Split('/');
                string currentPath = "";

                for (int i = 0; i < parts.Length - 1; i++) // Exclude file name
                {
                    if (string.IsNullOrEmpty(parts[i]))
                        continue;

                    currentPath = string.IsNullOrEmpty(currentPath) ? parts[i] : currentPath + "/" + parts[i];
                    
                    if (!folderSet.Contains(currentPath))
                    {
                        folderSet.Add(currentPath);
                        AddFolderToTree(rootFolder, currentPath, parts[i]);
                    }
                }
            }

            // Add empty folders that don't have resources
            foreach (var emptyFolderPath in emptyFolders)
            {
                if (string.IsNullOrEmpty(emptyFolderPath))
                    continue;

                // Check if folder already exists (has resources)
                if (!folderSet.Contains(emptyFolderPath))
                {
                    string[] parts = emptyFolderPath.Split('/');
                    string currentPath = "";
                    for (int i = 0; i < parts.Length; i++)
                    {
                        if (string.IsNullOrEmpty(parts[i]))
                            continue;

                        currentPath = string.IsNullOrEmpty(currentPath) ? parts[i] : currentPath + "/" + parts[i];
                        
                        if (!folderSet.Contains(currentPath))
                        {
                            folderSet.Add(currentPath);
                            AddFolderToTree(rootFolder, currentPath, parts[i]);
                        }
                    }
                }
            }

            Folders.Add(rootFolder);
        }

        private void OnProjectChanged(object? sender, EventArgs e)
        {
            System.Diagnostics.Debug.WriteLine($"OnProjectChanged: Project changed, current project: {_projectManager.CurrentProject?.ProjectPath ?? "null"}");
            Console.WriteLine($"OnProjectChanged: Project changed, current project: {_projectManager.CurrentProject?.ProjectPath ?? "null"}");
            
            // Ensure UI updates happen on UI thread
            System.Windows.Application.Current?.Dispatcher.InvokeAsync(() =>
            {
                RefreshResources();
            }, System.Windows.Threading.DispatcherPriority.Normal);
        }

        private void OnResourceSelected(ResourceBrowserItem? resource)
        {
            // Notify property panel about resource selection
            // This will be handled by MainViewModel
        }

        private void ApplyFilters()
        {
            FilteredResources.Clear();

            var filtered = Resources.AsEnumerable();

            // Apply type filter
            if (!string.IsNullOrEmpty(_typeFilter))
            {
                filtered = filtered.Where(r => r.Type.Equals(_typeFilter, StringComparison.OrdinalIgnoreCase));
            }

            // Apply search filter
            if (!string.IsNullOrEmpty(_searchText))
            {
                filtered = filtered.Where(r => 
                    r.Name.Contains(_searchText, StringComparison.OrdinalIgnoreCase) ||
                    r.VirtualPath.Contains(_searchText, StringComparison.OrdinalIgnoreCase));
            }

            // Apply folder filter
            if (!string.IsNullOrEmpty(_currentFolderPath))
            {
                filtered = filtered.Where(r => r.VirtualPath.StartsWith(_currentFolderPath, StringComparison.OrdinalIgnoreCase));
            }

            foreach (var item in filtered)
            {
                FilteredResources.Add(item);
            }
        }


        private void AddFolderToTree(FolderItem parent, string fullPath, string folderName)
        {
            string[] parts = fullPath.Split('/');
            FolderItem current = parent;

            foreach (string part in parts)
            {
                if (string.IsNullOrEmpty(part))
                    continue;

                // Build current path to check if this is the target folder
                string currentPath = string.IsNullOrEmpty(current.Path) ? part : current.Path + "/" + part;
                
                var child = current.Children?.FirstOrDefault(c => c.Name == part);
                if (child == null)
                {
                    child = new FolderItem
                    {
                        Name = part,
                        Path = currentPath,
                        Children = new ObservableCollection<FolderItem>()
                    };
                    
                    // Insert in sorted order to maintain consistent display order
                    if (current.Children == null)
                        current.Children = new ObservableCollection<FolderItem>();
                    
                    // Find insertion point to maintain alphabetical order
                    int insertIndex = 0;
                    for (int i = 0; i < current.Children.Count; i++)
                    {
                        if (string.Compare(current.Children[i].Name, part, StringComparison.OrdinalIgnoreCase) > 0)
                        {
                            insertIndex = i;
                            break;
                        }
                        insertIndex = i + 1;
                    }
                    
                    current.Children.Insert(insertIndex, child);
                }
                
                // Update path if it's not correct (for nested folders)
                if (child.Path != currentPath)
                {
                    child.Path = currentPath;
                }
                
                current = child;
            }
        }

        private string GetResourceDisplayName(ResourceEntry resource)
        {
            if (!string.IsNullOrEmpty(resource.VirtualPath))
            {
                string name = resource.VirtualPath.Split('/').Last();
                return name;
            }
            return Path.GetFileNameWithoutExtension(resource.Path);
        }

        private string? GetThumbnailPath(ResourceEntry resource, string packagePath)
        {
            // For textures, try to load the actual image as thumbnail
            if (resource.Type.Equals("Texture", StringComparison.OrdinalIgnoreCase))
            {
                // Parse XML to get image file
                string xmlPath = Path.Combine(packagePath, resource.Path);
                if (File.Exists(xmlPath))
                {
                    try
                    {
                        var doc = XDocument.Load(xmlPath);
                        var imageFile = doc.Root?.Element("ImageFile")?.Value;
                        if (!string.IsNullOrEmpty(imageFile))
                        {
                            string imagePath = Path.Combine(Path.GetDirectoryName(xmlPath) ?? "", imageFile);
                            if (File.Exists(imagePath))
                                return imagePath;
                        }
                    }
                    catch
                    {
                        // Ignore XML parsing errors
                    }
                }
            }

            // Return default icon path based on type
            return null; // Will use default icon
        }

        /// <summary>
        /// 扫描Shaders目录中的Shader文件并添加到资源列表
        /// </summary>
        private void ScanShaderFiles(string packagePath)
        {
            try
            {
                string shadersPath = Path.Combine(packagePath, "Shaders");
                if (!Directory.Exists(shadersPath))
                    return;

                // Supported shader extensions
                var shaderExtensions = new[] { ".hlsl", ".glsl", ".vert", ".frag", ".comp", ".geom", ".tesc", ".tese" };
                var shaderFiles = Directory.GetFiles(shadersPath, "*.*", SearchOption.AllDirectories)
                    .Where(f => shaderExtensions.Contains(Path.GetExtension(f).ToLower()))
                    .ToList();

                foreach (var shaderFile in shaderFiles)
                {
                    // Check if already in Resources (from manifest)
                    string relativePath = Path.GetRelativePath(packagePath, shaderFile).Replace('\\', '/');
                    bool alreadyExists = Resources.Any(r => 
                        r.Path.Equals(relativePath, StringComparison.OrdinalIgnoreCase) ||
                        r.Path.Equals(shaderFile, StringComparison.OrdinalIgnoreCase));

                    if (alreadyExists)
                        continue;

                    // Generate virtual path based on directory structure
                    string fileName = Path.GetFileNameWithoutExtension(shaderFile);
                    string parentDir = Path.GetDirectoryName(relativePath)?.Replace('\\', '/') ?? "Shaders";
                    string virtualPath;
                    
                    if (parentDir == "Shaders" || parentDir.EndsWith("/Shaders"))
                    {
                        virtualPath = $"shaders/{fileName.ToLower()}";
                    }
                    else
                    {
                        // Preserve subdirectory structure
                        string subPath = parentDir.Replace("Shaders/", "").Replace("Shaders", "");
                        if (string.IsNullOrEmpty(subPath))
                            virtualPath = $"shaders/{fileName.ToLower()}";
                        else
                            virtualPath = $"{subPath}/{fileName.ToLower()}";
                    }

                    // Generate a temporary ID (will be replaced when added to manifest)
                    int tempId = 9000 + Resources.Count;

                    var item = new ResourceBrowserItem
                    {
                        Id = tempId,
                        Name = fileName,
                        VirtualPath = virtualPath,
                        Path = relativePath,
                        Type = "Shader"
                    };

                    Resources.Add(item);
                    System.Diagnostics.Debug.WriteLine($"ScanShaderFiles: Added shader: {item.Name} (Path={item.Path}, VirtualPath={item.VirtualPath})");
                }

                Console.WriteLine($"ScanShaderFiles: Scanned {shaderFiles.Count} shader files");
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error scanning shader files: {ex.Message}");
                Console.WriteLine($"Error scanning shader files: {ex.Message}");
            }
        }

        private void ScanSceneFiles(string packagePath)
        {
            try
            {
                string scenesPath = Path.Combine(packagePath, "Scenes");
                if (!Directory.Exists(scenesPath))
                    return;

                var sceneFiles = Directory.GetFiles(scenesPath, "*.json", SearchOption.AllDirectories);
                foreach (var sceneFile in sceneFiles)
                {
                    try
                    {
                        // Check if already in resources
                        string relativePath = Path.GetRelativePath(packagePath, sceneFile).Replace('\\', '/');
                        bool alreadyExists = Resources.Any(r => r.Path.Equals(relativePath, StringComparison.OrdinalIgnoreCase));
                        
                        if (alreadyExists)
                            continue;

                        // Try to parse as scene data
                        string jsonContent = File.ReadAllText(sceneFile);
                        var sceneData = JsonConvert.DeserializeObject<SceneData>(jsonContent);
                        if (sceneData != null)
                        {
                            // Determine if it's a scene or scene level
                            string resourceType = (sceneData.Levels != null && sceneData.Levels.Count > 0) ? "Scene" : "SceneLevel";
                            string sceneName = sceneData.Name ?? Path.GetFileNameWithoutExtension(sceneFile);
                            
                            // Build virtual path
                            string virtualPath = $"Scenes/{sceneName}";
                            string parentDir = Path.GetDirectoryName(relativePath)?.Replace('\\', '/') ?? "Scenes";
                            if (parentDir != "Scenes")
                            {
                                virtualPath = $"{parentDir}/{sceneName}";
                            }

                            var item = new ResourceBrowserItem
                            {
                                Id = -1, // Temporary ID, should be assigned when imported
                                Name = sceneName,
                                VirtualPath = virtualPath,
                                Path = relativePath,
                                Type = resourceType,
                                ThumbnailPath = null
                            };

                            Resources.Add(item);
                            System.Diagnostics.Debug.WriteLine($"Added scene file: {item.Name} (Type={item.Type}, Path={item.Path})");
                        }
                    }
                    catch
                    {
                        // Skip invalid scene files
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error scanning scene files: {ex.Message}");
            }
        }
    }

    public class ResourceBrowserItem
    {
        public int Id { get; set; }
        public string Name { get; set; } = string.Empty;
        public string VirtualPath { get; set; } = string.Empty;
        public string Path { get; set; } = string.Empty;
        public string Type { get; set; } = string.Empty;
        public string? ThumbnailPath { get; set; }
        public string TypeIcon
        {
            get
            {
                return Type switch
                {
                    "Texture" => "🖼️",
                    "Model" => "🎲",
                    "Mesh" => "📐",
                    "Material" => "🎨",
                    "Scene" => "🌍",
                    "SceneLevel" => "📚",
                    "Shader" => "💻",
                    _ => "📄"
                };
            }
        }
    }

    public class FolderItem
    {
        public string Name { get; set; } = string.Empty;
        public string Path { get; set; } = string.Empty;
        public ObservableCollection<FolderItem>? Children { get; set; }
    }
}
