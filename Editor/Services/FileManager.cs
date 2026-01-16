using System.IO;

namespace FirstEngineEditor.Services
{
    public class FileManager : IFileManager
    {
        private string? _selectedFilePath;

        public string? GetSelectedFilePath()
        {
            return _selectedFilePath;
        }

        public void SetSelectedFilePath(string? filePath)
        {
            _selectedFilePath = filePath;
        }

        public bool CreateFile(string filePath, string content)
        {
            try
            {
                var directory = Path.GetDirectoryName(filePath);
                if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                File.WriteAllText(filePath, content);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public bool DeleteFile(string filePath)
        {
            try
            {
                if (File.Exists(filePath))
                {
                    File.Delete(filePath);
                    return true;
                }
                return false;
            }
            catch
            {
                return false;
            }
        }

        public bool RenameFile(string oldPath, string newPath)
        {
            try
            {
                if (File.Exists(oldPath))
                {
                    File.Move(oldPath, newPath);
                    return true;
                }
                return false;
            }
            catch
            {
                return false;
            }
        }
    }
}
