using System.Collections.Generic;
using System.Threading.Tasks;

namespace FirstEngineEditor.Services
{
    public interface IResourceImportService
    {
        // Import resource from file
        Task<bool> ImportResourceAsync(string sourceFilePath, string virtualPath, ResourceType resourceType);

        // Import resource with drag and drop
        Task<bool> ImportResourceFromDragDropAsync(string[] filePaths, string targetVirtualPath);

        // Create new resource (Scene, Material, etc.)
        Task<bool> CreateResourceAsync(string virtualPath, ResourceType resourceType, Dictionary<string, object>? properties = null);

        // Get supported file extensions for a resource type
        string[] GetSupportedExtensions(ResourceType resourceType);

        // Validate import file
        bool ValidateImportFile(string filePath, ResourceType resourceType);
    }

    public enum ResourceType
    {
        Texture,
        Model,
        Mesh,
        Material,
        Scene,
        SceneLevel,
        Light,
        Shader
    }
}
