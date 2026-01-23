using System.Threading.Tasks;

namespace FirstEngineEditor.Services
{
    public interface IFolderManagementService
    {
        /// <summary>
        /// 创建虚拟文件夹
        /// </summary>
        Task<bool> CreateFolderAsync(string parentVirtualPath, string folderName);

        /// <summary>
        /// 删除虚拟文件夹
        /// </summary>
        Task<bool> DeleteFolderAsync(string virtualPath);

        /// <summary>
        /// 重命名文件夹
        /// </summary>
        Task<bool> RenameFolderAsync(string virtualPath, string newName);

        /// <summary>
        /// 移动文件夹
        /// </summary>
        Task<bool> MoveFolderAsync(string sourceVirtualPath, string targetVirtualPath);

        /// <summary>
        /// 移动资源到新文件夹
        /// </summary>
        Task<bool> MoveResourceAsync(int resourceId, string targetVirtualPath);

        /// <summary>
        /// 验证虚拟路径是否有效
        /// </summary>
        bool IsValidVirtualPath(string virtualPath);
    }
}
