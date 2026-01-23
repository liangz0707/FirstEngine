using Newtonsoft.Json;

namespace FirstEngineEditor.Models
{
    public class ResourceManifest
    {
        [JsonProperty("version")]
        public int Version { get; set; }

        [JsonProperty("nextID")]
        public int NextID { get; set; }

        [JsonProperty("resources")]
        public List<ResourceEntry>? Resources { get; set; }

        [JsonProperty("emptyFolders")]
        public List<string>? EmptyFolders { get; set; }
    }

    public class ResourceEntry
    {
        [JsonProperty("id")]
        public int Id { get; set; }

        [JsonProperty("path")]
        public string Path { get; set; } = string.Empty;

        [JsonProperty("virtualPath")]
        public string? VirtualPath { get; set; }

        [JsonProperty("type")]
        public string Type { get; set; } = string.Empty;
    }
}