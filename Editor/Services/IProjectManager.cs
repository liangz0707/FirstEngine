namespace FirstEngineEditor.Services
{
    public interface IProjectManager
    {
        Project? CurrentProject { get; }
        
        event EventHandler? ProjectChanged;

        bool OpenProject(string projectPath);
        void CloseProject();
        bool CreateProject(string projectPath, string projectName);
        bool SaveProject();
    }
}
