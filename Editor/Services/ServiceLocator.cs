using System;
using System.Collections.Generic;

namespace FirstEngineEditor.Services
{
    public static class ServiceLocator
    {
        private static readonly Dictionary<Type, object> _services = new();

        public static void Initialize()
        {
            // Register services
            Register<IProjectManager>(new ProjectManager());
            Register<IFileManager>(new FileManager());
            Register<IResourceImportService>(new ResourceImportService());
            Register<IFolderManagementService>(new FolderManagementService());
            Register<ISceneManagementService>(new SceneManagementService());
        }

        public static void Shutdown()
        {
            _services.Clear();
        }

        public static void Register<T>(T service) where T : class
        {
            _services[typeof(T)] = service;
        }

        public static T Get<T>() where T : class
        {
            if (_services.TryGetValue(typeof(T), out var service))
            {
                return (T)service;
            }
            throw new InvalidOperationException($"Service {typeof(T)} not registered");
        }
    }
}
