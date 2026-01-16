using System.Collections.Generic;
using FirstEngineEditor.ViewModels;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class PropertyPanelViewModel : ViewModelBase
    {
        private object? _selectedObject;
        private List<PropertyItem> _properties = new();

        public PropertyPanelViewModel()
        {
            Properties = new List<PropertyItem>();
        }

        public object? SelectedObject
        {
            get => _selectedObject;
            set
            {
                SetProperty(ref _selectedObject, value);
                UpdateProperties();
            }
        }

        public List<PropertyItem> Properties
        {
            get => _properties;
            set => SetProperty(ref _properties, value);
        }

        private void UpdateProperties()
        {
            Properties.Clear();

            if (_selectedObject == null) return;

            // Use reflection to get properties
            var type = _selectedObject.GetType();
            var properties = type.GetProperties();

            foreach (var prop in properties)
            {
                var value = prop.GetValue(_selectedObject);
                Properties.Add(new PropertyItem
                {
                    Name = prop.Name,
                    Value = value?.ToString() ?? "null",
                    Type = prop.PropertyType.Name,
                    PropertyInfo = prop,
                    Object = _selectedObject
                });
            }
        }
    }

    public class PropertyItem
    {
        public string Name { get; set; } = string.Empty;
        public string Value { get; set; } = string.Empty;
        public string Type { get; set; } = string.Empty;
        public System.Reflection.PropertyInfo? PropertyInfo { get; set; }
        public object? Object { get; set; }
    }
}
