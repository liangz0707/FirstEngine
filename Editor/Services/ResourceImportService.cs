using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Xml.Linq;
using Newtonsoft.Json;
using FirstEngineEditor.Models;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.Services
{
    public class ResourceImportService : IResourceImportService
    {
        private readonly IProjectManager _projectManager;
        private readonly IFileManager _fileManager;
        private int _nextResourceID = 5000;

        public ResourceImportService()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
            _fileManager = ServiceLocator.Get<IFileManager>();
        }

        public async Task<bool> ImportResourceAsync(string sourceFilePath, string virtualPath, ResourceType resourceType)
        {
            if (string.IsNullOrEmpty(sourceFilePath) || !File.Exists(sourceFilePath))
                return false;

            if (_projectManager.CurrentProject == null)
                return false;

            try
            {
                return await Task.Run(() =>
                {
                    // Determine target directory based on resource type
                    string targetDir = GetTargetDirectory(resourceType);
                    string packageDir = Path.Combine(_projectManager.CurrentProject.ProjectPath, "Package");
                    
                    // Ensure Package directory exists
                    if (!Directory.Exists(packageDir))
                    {
                        System.Diagnostics.Debug.WriteLine($"ImportResourceAsync: Creating Package directory: {packageDir}");
                        Directory.CreateDirectory(packageDir);
                    }
                    
                    string fullTargetDir = Path.Combine(packageDir, targetDir);

                    if (!Directory.Exists(fullTargetDir))
                    {
                        System.Diagnostics.Debug.WriteLine($"ImportResourceAsync: Creating target directory: {fullTargetDir}");
                        Directory.CreateDirectory(fullTargetDir);
                    }

                    // Generate resource name from virtual path
                    string resourceName = Path.GetFileNameWithoutExtension(virtualPath);
                    if (string.IsNullOrEmpty(resourceName))
                        resourceName = Path.GetFileNameWithoutExtension(sourceFilePath);

                    // Copy source file to Package directory
                    string fileName = Path.GetFileName(sourceFilePath);
                    string targetFilePath = Path.Combine(fullTargetDir, fileName);
                    
                    System.Diagnostics.Debug.WriteLine($"ImportResourceAsync: Copying {sourceFilePath} to {targetFilePath}");
                    File.Copy(sourceFilePath, targetFilePath, true);

                    // Generate ResourceID
                    int resourceID = GetNextResourceID();

                    // For shaders, don't create XML - shaders are loaded directly from source files
                    if (resourceType == ResourceType.Shader)
                    {
                        // Shaders are registered in manifest but don't have XML descriptors
                        UpdateResourceManifest(resourceID, Path.Combine(targetDir, fileName).Replace('\\', '/'), virtualPath, resourceType);
                        return true;
                    }

                    // Create XML descriptor file
                    string xmlFileName = resourceName + GetResourceExtension(resourceType) + ".xml";
                    string xmlFilePath = Path.Combine(fullTargetDir, xmlFileName);

                    System.Diagnostics.Debug.WriteLine($"ImportResourceAsync: Creating XML descriptor: {xmlFilePath}");
                    CreateResourceXML(xmlFilePath, resourceName, resourceID, fileName, resourceType, virtualPath);

                    // Update resource manifest
                    string manifestPath = Path.Combine(targetDir, xmlFileName).Replace('\\', '/');
                    System.Diagnostics.Debug.WriteLine($"ImportResourceAsync: Updating manifest with resourceID={resourceID}, path={manifestPath}, virtualPath={virtualPath}");
                    UpdateResourceManifest(resourceID, manifestPath, virtualPath, resourceType);

                    System.Diagnostics.Debug.WriteLine($"ImportResourceAsync: Successfully imported resource {resourceName}");
                    return true;
                });
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error importing resource: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                Console.WriteLine($"Error importing resource: {ex.Message}");
                Console.WriteLine($"Stack trace: {ex.StackTrace}");
                return false;
            }
        }

        public async Task<bool> ImportResourceFromDragDropAsync(string[] filePaths, string targetVirtualPath)
        {
            if (filePaths == null || filePaths.Length == 0)
                return false;

            bool allSuccess = true;
            foreach (var filePath in filePaths)
            {
                if (!File.Exists(filePath))
                    continue;

                ResourceType resourceType = DetectResourceType(filePath);
                
                // Skip files that don't match any known resource type
                if (resourceType == ResourceType.Texture && !IsTextureFile(filePath))
                    continue;

                string virtualPath = targetVirtualPath;
                if (string.IsNullOrEmpty(virtualPath))
                {
                    // Generate virtual path from file name
                    string fileName = Path.GetFileNameWithoutExtension(filePath);
                    virtualPath = GetVirtualPathForType(resourceType) + "/" + fileName.ToLower();
                }

                bool success = await ImportResourceAsync(filePath, virtualPath, resourceType);
                if (!success)
                    allSuccess = false;
            }

            return allSuccess;
        }

        public async Task<bool> CreateResourceAsync(string virtualPath, ResourceType resourceType, Dictionary<string, object>? properties = null)
        {
            if (_projectManager.CurrentProject == null)
                return false;

            try
            {
                return await Task.Run(() =>
                {
                    string targetDir = GetTargetDirectory(resourceType);
                    string packageDir = Path.Combine(_projectManager.CurrentProject.ProjectPath, "Package");
                    string fullTargetDir = Path.Combine(packageDir, targetDir);

                    if (!Directory.Exists(fullTargetDir))
                        Directory.CreateDirectory(fullTargetDir);

                    string resourceName = Path.GetFileNameWithoutExtension(virtualPath);
                    if (string.IsNullOrEmpty(resourceName))
                        resourceName = "New" + resourceType.ToString();

                    int resourceID = GetNextResourceID();
                    
                    // For Scene, create JSON file instead of XML
                    if (resourceType == ResourceType.Scene)
                    {
                        string jsonFileName = resourceName + ".json";
                        string jsonFilePath = Path.Combine(fullTargetDir, jsonFileName);
                        CreateSceneJSON(jsonFilePath, resourceName, resourceID);
                        UpdateResourceManifest(resourceID, Path.Combine(targetDir, jsonFileName).Replace('\\', '/'), virtualPath, resourceType);
                    }
                    else
                    {
                        string xmlFileName = resourceName + GetResourceExtension(resourceType) + ".xml";
                        string xmlFilePath = Path.Combine(fullTargetDir, xmlFileName);
                        CreateResourceXML(xmlFilePath, resourceName, resourceID, "", resourceType, virtualPath, properties);
                        UpdateResourceManifest(resourceID, Path.Combine(targetDir, xmlFileName).Replace('\\', '/'), virtualPath, resourceType);
                    }

                    return true;
                });
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error creating resource: {ex.Message}");
                return false;
            }
        }

        public string[] GetSupportedExtensions(ResourceType resourceType)
        {
            return resourceType switch
            {
                ResourceType.Texture => new[] { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds", ".hdr", ".exr" },
                ResourceType.Model => new[] { ".fbx", ".obj", ".dae", ".3ds", ".blend", ".gltf", ".glb" },
                ResourceType.Mesh => new[] { ".obj", ".fbx", ".dae" },
                ResourceType.Shader => new[] { ".hlsl", ".glsl", ".vert", ".frag", ".comp" },
                ResourceType.Scene => new[] { ".json" },
                ResourceType.SceneLevel => new[] { ".json" },
                _ => Array.Empty<string>()
            };
        }

        public bool ValidateImportFile(string filePath, ResourceType resourceType)
        {
            if (!File.Exists(filePath))
                return false;

            string ext = Path.GetExtension(filePath).ToLower();
            string[] supported = GetSupportedExtensions(resourceType);
            return supported.Contains(ext);
        }

        private string GetTargetDirectory(ResourceType resourceType)
        {
            return resourceType switch
            {
                ResourceType.Texture => "Textures",
                ResourceType.Model => "Models",
                ResourceType.Mesh => "Meshes",
                ResourceType.Material => "Materials",
                ResourceType.Scene => "Scenes",
                ResourceType.SceneLevel => "Scenes",
                ResourceType.Shader => "Shaders",
                ResourceType.Light => "Lights",
                _ => "Misc"
            };
        }

        private string GetResourceExtension(ResourceType resourceType)
        {
            return resourceType switch
            {
                ResourceType.Texture => "_texture",
                ResourceType.Model => "_model",
                ResourceType.Mesh => "_mesh",
                ResourceType.Material => "_material",
                ResourceType.Scene => "_scene",
                ResourceType.SceneLevel => "_scenelevel",
                ResourceType.Shader => "", // Shaders don't use _shader suffix
                ResourceType.Light => "_light",
                _ => ""
            };
        }

        private string GetVirtualPathForType(ResourceType resourceType)
        {
            return resourceType switch
            {
                ResourceType.Texture => "textures",
                ResourceType.Model => "models",
                ResourceType.Mesh => "meshes",
                ResourceType.Material => "materials",
                ResourceType.Scene => "scenes",
                ResourceType.SceneLevel => "scenes",
                ResourceType.Shader => "shaders",
                ResourceType.Light => "lights",
                _ => "misc"
            };
        }

        private ResourceType DetectResourceType(string filePath)
        {
            string ext = Path.GetExtension(filePath).ToLower();
            
            if (IsTextureFile(filePath))
                return ResourceType.Texture;
            
            if (new[] { ".fbx", ".obj", ".dae", ".3ds", ".blend", ".gltf", ".glb" }.Contains(ext))
                return ResourceType.Model;
            
            if (new[] { ".hlsl", ".glsl", ".vert", ".frag", ".comp" }.Contains(ext))
                return ResourceType.Shader;

            // Detect scene files
            if (ext == ".json")
            {
                try
                {
                    // Try to parse JSON to determine if it's a scene or scene level
                    string jsonContent = File.ReadAllText(filePath);
                    var sceneData = JsonConvert.DeserializeObject<SceneData>(jsonContent);
                    if (sceneData != null)
                    {
                        // Check if it has levels (scene) or is a level itself
                        if (sceneData.Levels != null && sceneData.Levels.Count > 0)
                            return ResourceType.Scene;
                        else
                            return ResourceType.SceneLevel;
                    }
                }
                catch
                {
                    // Not a valid scene JSON, continue detection
                }
            }

            // Try to detect from file name pattern
            string fileName = Path.GetFileNameWithoutExtension(filePath).ToLower();
            if (fileName.Contains("mesh"))
                return ResourceType.Mesh;

            return ResourceType.Texture; // Default to texture for unknown types
        }

        private bool IsTextureFile(string filePath)
        {
            string ext = Path.GetExtension(filePath).ToLower();
            return new[] { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds", ".hdr", ".exr" }.Contains(ext);
        }

        private void CreateResourceXML(string xmlFilePath, string name, int resourceID, string sourceFile, ResourceType resourceType, string virtualPath, Dictionary<string, object>? properties = null)
        {
            XDocument doc = new XDocument();
            XElement root = new XElement(GetResourceRootElement(resourceType));
            
            root.Add(new XElement("Name", name));
            root.Add(new XElement("ResourceID", resourceID));

            switch (resourceType)
            {
                case ResourceType.Texture:
                    if (!string.IsNullOrEmpty(sourceFile))
                    {
                        root.Add(new XElement("ImageFile", sourceFile));
                        // Try to get image dimensions (would need image loading library)
                        root.Add(new XElement("Width", "512"));
                        root.Add(new XElement("Height", "512"));
                        root.Add(new XElement("Channels", "4"));
                        root.Add(new XElement("HasAlpha", "true"));
                    }
                    break;

                case ResourceType.Mesh:
                    if (!string.IsNullOrEmpty(sourceFile))
                    {
                        root.Add(new XElement("MeshFile", sourceFile));
                        root.Add(new XElement("VertexStride", "32"));
                    }
                    break;

                case ResourceType.Material:
                    root.Add(new XElement("Shader", properties?.ContainsKey("Shader") == true ? properties["Shader"].ToString() : "PBR"));
                    var parametersNode = new XElement("Parameters");
                    // Add default PBR parameters
                    var baseColorParam = new XElement("Parameter");
                    baseColorParam.SetAttributeValue("name", "baseColor");
                    baseColorParam.SetAttributeValue("type", "float4");
                    baseColorParam.SetValue("1.0 1.0 1.0 1.0");
                    parametersNode.Add(baseColorParam);
                    
                    var metallicParam = new XElement("Parameter");
                    metallicParam.SetAttributeValue("name", "metallic");
                    metallicParam.SetAttributeValue("type", "float");
                    metallicParam.SetValue("0.0");
                    parametersNode.Add(metallicParam);
                    
                    var roughnessParam = new XElement("Parameter");
                    roughnessParam.SetAttributeValue("name", "roughness");
                    roughnessParam.SetAttributeValue("type", "float");
                    roughnessParam.SetValue("0.5");
                    parametersNode.Add(roughnessParam);
                    
                    root.Add(parametersNode);
                    root.Add(new XElement("Textures"));
                    root.Add(new XElement("Dependencies"));
                    break;

                case ResourceType.Model:
                    root.Add(new XElement("Meshes"));
                    root.Add(new XElement("Materials"));
                    root.Add(new XElement("Dependencies"));
                    break;

                case ResourceType.Scene:
                    root.Add(new XElement("Entities"));
                    var levelsNode = new XElement("Levels");
                    // Add default level
                    var defaultLevel = new XElement("Level");
                    defaultLevel.Add(new XAttribute("name", "Default"));
                    defaultLevel.Add(new XAttribute("order", "0"));
                    levelsNode.Add(defaultLevel);
                    root.Add(levelsNode);
                    break;

                case ResourceType.SceneLevel:
                    root.Add(new XElement("Name", name + "_Level"));
                    root.Add(new XElement("Order", "0"));
                    root.Add(new XElement("Entities"));
                    break;

                case ResourceType.Light:
                    root.Add(new XElement("LightType", "Point"));
                    root.Add(new XElement("Color", "1.0 1.0 1.0"));
                    root.Add(new XElement("Intensity", "1.0"));
                    root.Add(new XElement("Range", "10.0"));
                    break;

                case ResourceType.Shader:
                    if (!string.IsNullOrEmpty(sourceFile))
                    {
                        root.Add(new XElement("ShaderFile", sourceFile));
                        // Detect shader stage from file extension
                        string ext = Path.GetExtension(sourceFile).ToLower();
                        string stage = "vertex";
                        if (ext.Contains("frag") || ext.Contains("pixel"))
                            stage = "fragment";
                        else if (ext.Contains("comp") || ext.Contains("compute"))
                            stage = "compute";
                        root.Add(new XElement("Stage", stage));
                    }
                    break;
            }

            doc.Add(root);
            doc.Save(xmlFilePath);
        }

        private string GetResourceRootElement(ResourceType resourceType)
        {
            return resourceType switch
            {
                ResourceType.Texture => "Texture",
                ResourceType.Mesh => "Mesh",
                ResourceType.Material => "Material",
                ResourceType.Model => "Model",
                ResourceType.Scene => "Scene",
                ResourceType.Shader => "Shader",
                _ => "Resource"
            };
        }

        private void UpdateResourceManifest(int resourceID, string path, string virtualPath, ResourceType resourceType)
        {
            if (_projectManager.CurrentProject == null)
            {
                System.Diagnostics.Debug.WriteLine("UpdateResourceManifest: No current project");
                return;
            }

            string packageDir = Path.Combine(_projectManager.CurrentProject.ProjectPath, "Package");
            if (!Directory.Exists(packageDir))
            {
                System.Diagnostics.Debug.WriteLine($"UpdateResourceManifest: Package directory does not exist: {packageDir}");
                Directory.CreateDirectory(packageDir);
            }

            string manifestPath = Path.Combine(packageDir, "resource_manifest.json");
            
            ResourceManifest manifest;
            if (File.Exists(manifestPath))
            {
                try
                {
                    string json = File.ReadAllText(manifestPath);
                    manifest = JsonConvert.DeserializeObject<ResourceManifest>(json) ?? new ResourceManifest 
                    { 
                        Version = 1, 
                        NextID = 5000, 
                        Resources = new List<ResourceEntry>(),
                        EmptyFolders = new List<string>()
                    };
                    System.Diagnostics.Debug.WriteLine($"UpdateResourceManifest: Loaded existing manifest with {manifest.Resources?.Count ?? 0} resources");
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"UpdateResourceManifest: Error loading manifest: {ex.Message}");
                    manifest = new ResourceManifest();
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine("UpdateResourceManifest: Creating new manifest");
                manifest = new ResourceManifest 
                { 
                    Version = 1, 
                    NextID = 5000, 
                    Resources = new List<ResourceEntry>(),
                    EmptyFolders = new List<string>()
                };
            }

            if (manifest.Resources == null)
                manifest.Resources = new List<ResourceEntry>();
            
            if (manifest.EmptyFolders == null)
                manifest.EmptyFolders = new List<string>();

            // Check if resource already exists
            var existing = manifest.Resources.FirstOrDefault(r => r.Id == resourceID || r.VirtualPath == virtualPath);
            if (existing != null)
            {
                existing.Path = path;
                existing.VirtualPath = virtualPath;
                existing.Type = resourceType.ToString();
            }
            else
            {
                manifest.Resources.Add(new ResourceEntry
                {
                    Id = resourceID,
                    Path = path,
                    VirtualPath = virtualPath,
                    Type = resourceType.ToString()
                });
            }

            // 如果资源被导入到某个文件夹，从空文件夹列表中移除该文件夹
            if (!string.IsNullOrEmpty(virtualPath))
            {
                // 提取父文件夹路径
                int lastSlash = virtualPath.LastIndexOf('/');
                if (lastSlash >= 0)
                {
                    string parentFolder = virtualPath.Substring(0, lastSlash);
                    manifest.EmptyFolders.RemoveAll(f => 
                        f.Equals(parentFolder, StringComparison.OrdinalIgnoreCase));
                }
            }

            manifest.NextID = Math.Max(manifest.NextID, resourceID + 1);

            string jsonOutput = JsonConvert.SerializeObject(manifest, Formatting.Indented);
            File.WriteAllText(manifestPath, jsonOutput);
        }

        private int GetNextResourceID()
        {
            if (_projectManager.CurrentProject == null)
                return _nextResourceID++;

            string manifestPath = Path.Combine(_projectManager.CurrentProject.ProjectPath, "Package", "resource_manifest.json");
            if (File.Exists(manifestPath))
            {
                try
                {
                    string json = File.ReadAllText(manifestPath);
                    var manifest = JsonConvert.DeserializeObject<ResourceManifest>(json);
                    if (manifest != null && manifest.NextID > 0)
                    {
                        _nextResourceID = manifest.NextID;
                        return _nextResourceID++;
                    }
                }
                catch
                {
                    // Use default
                }
            }

            return _nextResourceID++;
        }

        private void CreateSceneJSON(string jsonFilePath, string name, int resourceID)
        {
            var sceneData = new
            {
                version = 1,
                name = name,
                resourceID = resourceID,
                levels = new[]
                {
                    new
                    {
                        name = "Default",
                        order = 0,
                        visible = true,
                        enabled = true,
                        entities = new object[0]
                    }
                }
            };

            string json = JsonConvert.SerializeObject(sceneData, Formatting.Indented);
            File.WriteAllText(jsonFilePath, json);
        }
    }
}
