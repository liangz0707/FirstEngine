using System;
using System.Threading.Tasks;

namespace FirstEngineEditor.Services
{
    public interface ISceneManagementService
    {
        /// <summary>
        /// 当前已加载的场景文件路径
        /// </summary>
        string? CurrentScenePath { get; }

        /// <summary>
        /// 场景已加载事件
        /// </summary>
        event EventHandler<SceneLoadedEventArgs>? SceneLoaded;

        /// <summary>
        /// 场景卸载事件
        /// </summary>
        event EventHandler? SceneUnloaded;

        /// <summary>
        /// 加载场景文件
        /// </summary>
        Task<bool> LoadSceneAsync(string scenePath);

        /// <summary>
        /// 加载场景分层（需要先有已加载的场景）
        /// </summary>
        Task<bool> LoadSceneLevelAsync(string levelPath);

        /// <summary>
        /// 卸载当前场景
        /// </summary>
        Task UnloadSceneAsync();

        /// <summary>
        /// 保存当前场景到文件
        /// </summary>
        Task<bool> SaveSceneAsync(string? scenePath = null);

        /// <summary>
        /// 获取当前场景的文件路径（如果有）
        /// </summary>
        string? GetCurrentScenePath();
    }

    public class SceneLoadedEventArgs : EventArgs
    {
        public string ScenePath { get; }
        public bool IsNewScene { get; }

        public SceneLoadedEventArgs(string scenePath, bool isNewScene)
        {
            ScenePath = scenePath;
            IsNewScene = isNewScene;
        }
    }
}
