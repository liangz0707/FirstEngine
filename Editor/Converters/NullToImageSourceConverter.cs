using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace FirstEngineEditor.Converters
{
    public class NullToImageSourceConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is string str && !string.IsNullOrEmpty(str))
            {
                try
                {
                    return new System.Windows.Media.Imaging.BitmapImage(new Uri(str, UriKind.RelativeOrAbsolute));
                }
                catch
                {
                    return DependencyProperty.UnsetValue;
                }
            }
            return DependencyProperty.UnsetValue;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
