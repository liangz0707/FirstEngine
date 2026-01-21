using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels.Panels;
using Microsoft.Win32;

namespace FirstEngineEditor.Views.Panels
{
    public partial class ResourceBrowserView : System.Windows.Controls.UserControl
    {
        private ResourceBrowserViewModel? _viewModel;
        private IResourceImportService? _importService;

        public ResourceBrowserView()
        {
            InitializeComponent();
            Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            _viewModel = DataContext as ResourceBrowserViewModel;
            _importService = ServiceLocator.Get<IResourceImportService>();
            
            System.Diagnostics.Debug.WriteLine($"ResourceBrowserView.OnLoaded: ViewModel is {(_viewModel != null ? "not null" : "null")}");
            Console.WriteLine($"ResourceBrowserView.OnLoaded: ViewModel is {(_viewModel != null ? "not null" : "null")}");
            
            if (_viewModel != null)
            {
                // Force refresh after a short delay to ensure project is loaded
                Dispatcher.BeginInvoke(new System.Action(() =>
                {
                    _viewModel.RefreshResources();
                    System.Diagnostics.Debug.WriteLine($"ResourceBrowserView: Forced refresh after load");
                }), System.Windows.Threading.DispatcherPriority.Loaded);
            }
        }

        private void OnImportResource(object sender, RoutedEventArgs e)
        {
            if (_importService == null) return;

            var dialog = new Views.ResourceImportDialog();
            if (dialog.ShowDialog() == true)
            {
                // Import will be handled by dialog
                _viewModel?.RefreshResources();
            }
        }

        private void OnCreateResource(object sender, RoutedEventArgs e)
        {
            if (_importService == null || _viewModel == null) return;

            var dialog = new Views.CreateResourceDialog();
            if (dialog.ShowDialog() == true)
            {
                _viewModel.RefreshResources();
            }
        }

        private void OnRefresh(object sender, RoutedEventArgs e)
        {
            System.Diagnostics.Debug.WriteLine("OnRefresh: Manual refresh triggered");
            Console.WriteLine("OnRefresh: Manual refresh triggered");
            _viewModel?.RefreshResources();
            
            // Show status
            if (_viewModel != null)
            {
                int resourceCount = _viewModel.Resources.Count;
                int filteredCount = _viewModel.FilteredResources.Count;
                System.Windows.MessageBox.Show($"Resources loaded: {resourceCount}\nFiltered: {filteredCount}", 
                    "Refresh Complete", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
            }
        }

        private void OnTypeFilterChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_viewModel == null) return;

            var selectedItem = TypeFilterComboBox.SelectedItem as ComboBoxItem;
            string filter = selectedItem?.Content?.ToString() ?? "All Types";
            _viewModel.FilterByType(filter == "All Types" ? null : filter);
        }

        private void OnSearchTextChanged(object sender, TextChangedEventArgs e)
        {
            if (_viewModel == null) return;

            string searchText = SearchTextBox.Text;
            _viewModel.SearchResources(searchText);
        }

        private void OnFolderSelected(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            if (_viewModel == null) return;

            var selectedItem = e.NewValue as FolderItem;
            if (selectedItem != null)
            {
                _viewModel.NavigateToFolder(selectedItem.Path);
            }
        }

        private void OnNavigateToRoot(object sender, RoutedEventArgs e)
        {
            _viewModel?.NavigateToFolder("");
        }

        private void OnResourceItemClicked(object sender, MouseButtonEventArgs e)
        {
            if (sender is FrameworkElement element && element.DataContext is ResourceBrowserItem item)
            {
                _viewModel?.SelectResource(item);
            }
        }

        private void OnDragOver(object sender, System.Windows.DragEventArgs e)
        {
            if (e.Data.GetDataPresent(System.Windows.DataFormats.FileDrop))
            {
                e.Effects = System.Windows.DragDropEffects.Copy;
            }
            else
            {
                e.Effects = System.Windows.DragDropEffects.None;
            }
        }

        private async void OnDrop(object sender, System.Windows.DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(System.Windows.DataFormats.FileDrop) || _importService == null)
                return;

            string[] files = (string[])e.Data.GetData(System.Windows.DataFormats.FileDrop);
            if (files == null || files.Length == 0)
                return;

            // Get current folder path from view model
            string targetPath = _viewModel?.CurrentFolderPath ?? "";

            bool success = await _importService.ImportResourceFromDragDropAsync(files, targetPath);
            if (success)
            {
                _viewModel?.RefreshResources();
                System.Windows.MessageBox.Show($"Successfully imported {files.Length} resource(s)", "Import Success", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
            }
            else
            {
                System.Windows.MessageBox.Show("Failed to import some resources", "Import Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }
    }
}
