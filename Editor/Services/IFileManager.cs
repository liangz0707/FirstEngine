namespace FirstEngineEditor.Services
{
    public interface IFileManager
    {
        string? GetSelectedFilePath();
        void SetSelectedFilePath(string? filePath);
        bool CreateFile(string filePath, string content);
        bool DeleteFile(string filePath);
        bool RenameFile(string oldPath, string newPath);
    }
}
