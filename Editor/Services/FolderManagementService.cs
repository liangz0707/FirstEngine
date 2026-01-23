using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Newtonsoft.Json;
using FirstEngineEditor.Models;

namespace FirstEngineEditor.Services
{
    public class FolderManagementService : IFolderManagementService
    {
        private readonly IProjectManager _projectManager;

        public FolderManagementService()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
        }

        public async Task<bool> CreateFolderAsync(string parentVirtualPath, string folderName)
        {
            if (string.IsNullOrEmpty(folderName))
                return false;

            var project = _projectManager.CurrentProject;
            if (project == null)
                return false;

            return await Task.Run(() =>
            {
                try
                {
                    // 验证文件夹名称
                    if (folderName.Contains('/') || folderName.Contains('\\'))
                        return false;

                    // 构建新的虚拟路径
                    string newVirtualPath = string.IsNullOrEmpty(parentVirtualPath)
                        ? folderName
                        : $"{parentVirtualPath}/{folderName}";

                    string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
                    if (string.IsNullOrEmpty(packagePath))
                        return false;

                    string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
                    if (!File.Exists(manifestPath))
                        return false;

                    // 加载manifest
                    var manifest = LoadManifest(manifestPath);
                    
                    // 检查文件夹是否已存在（在资源路径中或空文件夹列表中）
                    bool folderExists = false;
                    if (manifest.Resources != null)
                    {
                        folderExists = manifest.Resources.Any(r => 
                            !string.IsNullOrEmpty(r.VirtualPath) && 
                            (r.VirtualPath.Equals(newVirtualPath, StringComparison.OrdinalIgnoreCase) ||
                             r.VirtualPath.StartsWith(newVirtualPath + "/", StringComparison.OrdinalIgnoreCase)));
                    }
                    
                    if (!folderExists)
                    {
                        // 添加到空文件夹列表
                        if (manifest.EmptyFolders == null)
                            manifest.EmptyFolders = new List<string>();
                        
                        // 检查是否已在空文件夹列表中
                        if (!manifest.EmptyFolders.Contains(newVirtualPath, StringComparer.OrdinalIgnoreCase))
                        {
                            manifest.EmptyFolders.Add(newVirtualPath);
                            SaveManifest(manifestPath, manifest);
                            System.Diagnostics.Debug.WriteLine($"CreateFolderAsync: Created empty folder: {newVirtualPath}");
                        }
                    }

                    return true;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error creating folder: {ex.Message}");
                    return false;
                }
            });
        }

        public async Task<bool> DeleteFolderAsync(string virtualPath)
        {
            var project = _projectManager.CurrentProject;
            if (project == null)
                return false;

            return await Task.Run(() =>
            {
                try
                {
                    string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
                    if (string.IsNullOrEmpty(packagePath))
                        return false;

                    string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
                    if (!File.Exists(manifestPath))
                        return false;

                    // 加载manifest
                    var manifest = LoadManifest(manifestPath);
                    if (manifest?.Resources == null)
                        return false;

                    // 确保 EmptyFolders 列表存在
                    if (manifest.EmptyFolders == null)
                        manifest.EmptyFolders = new List<string>();

                    // 查找所有在该文件夹下的资源
                    var resourcesToRemove = manifest.Resources
                        .Where(r => !string.IsNullOrEmpty(r.VirtualPath) && 
                                   r.VirtualPath.StartsWith(virtualPath + "/", StringComparison.OrdinalIgnoreCase))
                        .ToList();

                    if (resourcesToRemove.Count > 0)
                    {
                        // 询问用户是否确认删除
                        // 这里简化处理，直接删除
                        foreach (var resource in resourcesToRemove)
                        {
                            manifest.Resources.Remove(resource);
                        }
                    }

                    // 从空文件夹列表中移除该文件夹及其子文件夹
                    manifest.EmptyFolders.RemoveAll(f => 
                        f.Equals(virtualPath, StringComparison.OrdinalIgnoreCase) ||
                        f.StartsWith(virtualPath + "/", StringComparison.OrdinalIgnoreCase));

                    // 保存manifest
                    SaveManifest(manifestPath, manifest);
                    System.Diagnostics.Debug.WriteLine($"DeleteFolderAsync: Deleted folder: {virtualPath}");
                    return true;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error deleting folder: {ex.Message}");
                    return false;
                }
            });
        }

        public async Task<bool> RenameFolderAsync(string virtualPath, string newName)
        {
            if (string.IsNullOrEmpty(newName))
                return false;

            var project = _projectManager.CurrentProject;
            if (project == null)
                return false;

            return await Task.Run(() =>
            {
                try
                {
                    string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
                    if (string.IsNullOrEmpty(packagePath))
                        return false;

                    string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
                    if (!File.Exists(manifestPath))
                        return false;

                    var manifest = LoadManifest(manifestPath);
                    if (manifest?.Resources == null)
                        return false;

                    // 确保 EmptyFolders 列表存在
                    if (manifest.EmptyFolders == null)
                        manifest.EmptyFolders = new List<string>();

                    // 构建新的虚拟路径前缀
                    string parentPath = string.IsNullOrEmpty(virtualPath) ? "" : virtualPath.Substring(0, virtualPath.LastIndexOf('/'));
                    string newVirtualPathPrefix = string.IsNullOrEmpty(parentPath) ? newName : $"{parentPath}/{newName}";

                    // 更新所有相关资源的虚拟路径
                    foreach (var resource in manifest.Resources)
                    {
                        if (!string.IsNullOrEmpty(resource.VirtualPath) && 
                            resource.VirtualPath.StartsWith(virtualPath + "/", StringComparison.OrdinalIgnoreCase))
                        {
                            string relativePath = resource.VirtualPath.Substring(virtualPath.Length);
                            resource.VirtualPath = newVirtualPathPrefix + relativePath;
                        }
                        else if (resource.VirtualPath?.Equals(virtualPath, StringComparison.OrdinalIgnoreCase) == true)
                        {
                            resource.VirtualPath = newVirtualPathPrefix;
                        }
                    }

                    // 更新空文件夹列表中的相关路径
                    for (int i = manifest.EmptyFolders.Count - 1; i >= 0; i--)
                    {
                        string emptyFolderPath = manifest.EmptyFolders[i];
                        if (emptyFolderPath.Equals(virtualPath, StringComparison.OrdinalIgnoreCase))
                        {
                            // 重命名的文件夹本身是空文件夹
                            manifest.EmptyFolders[i] = newVirtualPathPrefix;
                        }
                        else if (emptyFolderPath.StartsWith(virtualPath + "/", StringComparison.OrdinalIgnoreCase))
                        {
                            // 重命名文件夹的子文件夹
                            string relativePath = emptyFolderPath.Substring(virtualPath.Length);
                            manifest.EmptyFolders[i] = newVirtualPathPrefix + relativePath;
                        }
                    }

                    SaveManifest(manifestPath, manifest);
                    System.Diagnostics.Debug.WriteLine($"RenameFolderAsync: Renamed folder from {virtualPath} to {newVirtualPathPrefix}");
                    return true;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error renaming folder: {ex.Message}");
                    return false;
                }
            });
        }

        public async Task<bool> MoveFolderAsync(string sourceVirtualPath, string targetVirtualPath)
        {
            var project = _projectManager.CurrentProject;
            if (project == null)
                return false;

            return await Task.Run(() =>
            {
                try
                {
                    string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
                    if (string.IsNullOrEmpty(packagePath))
                        return false;

                    string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
                    if (!File.Exists(manifestPath))
                        return false;

                    var manifest = LoadManifest(manifestPath);
                    if (manifest?.Resources == null)
                        return false;

                    // 确保 EmptyFolders 列表存在
                    if (manifest.EmptyFolders == null)
                        manifest.EmptyFolders = new List<string>();

                    // 获取源文件夹名称
                    string folderName = sourceVirtualPath.Split('/').Last();

                    // 构建新的虚拟路径前缀
                    string newVirtualPathPrefix = string.IsNullOrEmpty(targetVirtualPath) 
                        ? folderName 
                        : $"{targetVirtualPath}/{folderName}";

                    // 更新所有相关资源的虚拟路径
                    foreach (var resource in manifest.Resources)
                    {
                        if (!string.IsNullOrEmpty(resource.VirtualPath))
                        {
                            if (resource.VirtualPath.StartsWith(sourceVirtualPath + "/", StringComparison.OrdinalIgnoreCase))
                            {
                                string relativePath = resource.VirtualPath.Substring(sourceVirtualPath.Length);
                                resource.VirtualPath = newVirtualPathPrefix + relativePath;
                            }
                            else if (resource.VirtualPath.Equals(sourceVirtualPath, StringComparison.OrdinalIgnoreCase))
                            {
                                resource.VirtualPath = newVirtualPathPrefix;
                            }
                        }
                    }

                    // 更新空文件夹列表中的相关路径
                    for (int i = manifest.EmptyFolders.Count - 1; i >= 0; i--)
                    {
                        string emptyFolderPath = manifest.EmptyFolders[i];
                        if (emptyFolderPath.Equals(sourceVirtualPath, StringComparison.OrdinalIgnoreCase))
                        {
                            // 源文件夹本身是空文件夹，移动到新位置
                            manifest.EmptyFolders[i] = newVirtualPathPrefix;
                        }
                        else if (emptyFolderPath.StartsWith(sourceVirtualPath + "/", StringComparison.OrdinalIgnoreCase))
                        {
                            // 源文件夹的子文件夹，更新路径
                            string relativePath = emptyFolderPath.Substring(sourceVirtualPath.Length);
                            manifest.EmptyFolders[i] = newVirtualPathPrefix + relativePath;
                        }
                    }

                    SaveManifest(manifestPath, manifest);
                    System.Diagnostics.Debug.WriteLine($"MoveFolderAsync: Moved folder from {sourceVirtualPath} to {newVirtualPathPrefix}");
                    return true;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error moving folder: {ex.Message}");
                    return false;
                }
            });
        }

        public async Task<bool> MoveResourceAsync(int resourceId, string targetVirtualPath)
        {
            var project = _projectManager.CurrentProject;
            if (project == null)
                return false;

            return await Task.Run(() =>
            {
                try
                {
                    string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
                    if (string.IsNullOrEmpty(packagePath))
                        return false;

                    string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
                    if (!File.Exists(manifestPath))
                        return false;

                    var manifest = LoadManifest(manifestPath);
                    if (manifest?.Resources == null)
                        return false;

                    // 查找资源
                    var resource = manifest.Resources.FirstOrDefault(r => r.Id == resourceId);
                    if (resource == null)
                        return false;

                    // 获取资源名称
                    string resourceName = resource.VirtualPath?.Split('/').Last() ?? resource.Path.Split('/').Last();

                    // 更新虚拟路径
                    string newVirtualPath = string.IsNullOrEmpty(targetVirtualPath) 
                        ? resourceName 
                        : $"{targetVirtualPath}/{resourceName}";
                    
                    resource.VirtualPath = newVirtualPath;

                    // 如果目标文件夹在空文件夹列表中，移除它（因为现在有资源了）
                    if (manifest.EmptyFolders != null && !string.IsNullOrEmpty(targetVirtualPath))
                    {
                        manifest.EmptyFolders.RemoveAll(f => 
                            f.Equals(targetVirtualPath, StringComparison.OrdinalIgnoreCase));
                    }

                    SaveManifest(manifestPath, manifest);
                    System.Diagnostics.Debug.WriteLine($"MoveResourceAsync: Moved resource {resourceId} to {resource.VirtualPath}");
                    return true;
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error moving resource: {ex.Message}");
                    return false;
                }
            });
        }

        public bool IsValidVirtualPath(string virtualPath)
        {
            if (string.IsNullOrEmpty(virtualPath))
                return true; // 根路径是有效的

            // 检查路径格式
            if (virtualPath.Contains("\\") || virtualPath.StartsWith("/") || virtualPath.EndsWith("/"))
                return false;

            // 检查是否有连续的分隔符
            if (virtualPath.Contains("//"))
                return false;

            return true;
        }

        private Models.ResourceManifest LoadManifest(string manifestPath)
        {
            try
            {
                string jsonContent = File.ReadAllText(manifestPath);
                return JsonConvert.DeserializeObject<Models.ResourceManifest>(jsonContent) ?? new Models.ResourceManifest
                {
                    Version = 1,
                    NextID = 5000,
                    Resources = new List<Models.ResourceEntry>(),
                    EmptyFolders = new List<string>()
                };
            }
            catch
            {
                return new Models.ResourceManifest
                {
                    Version = 1,
                    NextID = 5000,
                    Resources = new List<Models.ResourceEntry>(),
                    EmptyFolders = new List<string>()
                };
            }
        }

        private void SaveManifest(string manifestPath, Models.ResourceManifest manifest)
        {
            try
            {
                string jsonContent = JsonConvert.SerializeObject(manifest, Formatting.Indented);
                File.WriteAllText(manifestPath, jsonContent);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error saving manifest: {ex.Message}");
                throw;
            }
        }
    }
}
