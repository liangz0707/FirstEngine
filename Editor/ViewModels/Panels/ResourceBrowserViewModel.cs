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

            // Build folder structure
            BuildFolderTree(packagePath);

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

        private void BuildFolderTree(string packagePath)
        {
            var rootFolder = new FolderItem
            {
                Name = "Package",
                Path = "",
                Children = new ObservableCollection<FolderItem>()
            };

            // Build folder structure from virtual paths
            var folderSet = new HashSet<string>();
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

            Folders.Add(rootFolder);
        }

        private void AddFolderToTree(FolderItem parent, string fullPath, string folderName)
        {
            string[] parts = fullPath.Split('/');
            FolderItem current = parent;

            foreach (string part in parts)
            {
                if (string.IsNullOrEmpty(part))
                    continue;

                var child = current.Children?.FirstOrDefault(c => c.Name == part);
                if (child == null)
                {
                    child = new FolderItem
                    {
                        Name = part,
                        Path = fullPath,
                        Children = new ObservableCollection<FolderItem>()
                    };
                    current.Children?.Add(child);
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
