using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using FirstEngineEditor.Models;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels.Panels;
using Microsoft.Win32;

namespace FirstEngineEditor.Views.Panels
{
    public partial class ResourceBrowserView : System.Windows.Controls.UserControl
    {
        private ResourceBrowserViewModel? _viewModel;
        private IResourceImportService? _importService;
        private IFolderManagementService? _folderManagementService;
        
        // Drag and drop state
        private ResourceBrowserItem? _draggedResource;
        private FolderItem? _draggedFolder;
        private System.Windows.Point _dragStartPoint;
        private bool _isDragging;

        public ResourceBrowserView()
        {
            InitializeComponent();
            Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            _viewModel = DataContext as ResourceBrowserViewModel;
            _importService = ServiceLocator.Get<IResourceImportService>();
            _folderManagementService = ServiceLocator.Get<IFolderManagementService>();
            
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
            try
            {
                if (_importService == null) return;

                var dialog = new Views.ResourceImportDialog();
                if (dialog.ShowDialog() == true)
                {
                    // Import will be handled by dialog
                    _viewModel?.RefreshResources();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnImportResource: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error importing resource: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnCreateResource(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_importService == null || _viewModel == null) return;

                var dialog = new Views.CreateResourceDialog();
                if (dialog.ShowDialog() == true)
                {
                    _viewModel.RefreshResources();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnCreateResource: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error creating resource: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnRefresh(object sender, RoutedEventArgs e)
        {
            try
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
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnRefresh: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error refreshing resources: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
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

        // Resource drag and drop handlers
        private void OnResourceItemMouseMove(object sender, System.Windows.Input.MouseEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed && !_isDragging)
            {
                if (sender is FrameworkElement element && element.DataContext is ResourceBrowserItem item)
                {
                    System.Windows.Point currentPoint = e.GetPosition(null);
                    if (Math.Abs(currentPoint.X - _dragStartPoint.X) > SystemParameters.MinimumHorizontalDragDistance ||
                        Math.Abs(currentPoint.Y - _dragStartPoint.Y) > SystemParameters.MinimumVerticalDragDistance)
                    {
                        _draggedResource = item;
                        _isDragging = true;
                        
                        // Use Link effect for Scene/SceneLevel to allow dropping on render view
                        System.Windows.DragDropEffects effects = (item.Type == "Scene" || item.Type == "SceneLevel") 
                            ? System.Windows.DragDropEffects.Link 
                            : System.Windows.DragDropEffects.Move;
                        
                        System.Windows.DataObject dataObject = new System.Windows.DataObject("ResourceBrowserItem", item);
                        DragDrop.DoDragDrop(element, dataObject, effects);
                        
                        _isDragging = false;
                        _draggedResource = null;
                    }
                }
            }
        }

        private void OnResourceItemMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _dragStartPoint = e.GetPosition(null);
            if (sender is FrameworkElement element && element.DataContext is ResourceBrowserItem item)
            {
                _viewModel?.SelectResource(item);
            }
        }

        private void OnResourceItemMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            _isDragging = false;
            _draggedResource = null;
        }

        // Folder tree drag and drop handlers
        private void OnFolderTreeDragOver(object sender, System.Windows.DragEventArgs e)
        {
            if (_folderManagementService == null || _viewModel == null) return;

            // Check if dragging a resource
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                e.Effects = System.Windows.DragDropEffects.Move;
                e.Handled = true;
                return;
            }

            // Check if dragging a folder
            if (e.Data.GetDataPresent("FolderItem"))
            {
                var draggedFolder = e.Data.GetData("FolderItem") as FolderItem;
                if (draggedFolder != null)
                {
                    // Get target folder from tree view item
                    var targetTreeViewItem = GetTreeViewItemUnderPoint((System.Windows.Controls.TreeView)sender, e.GetPosition((System.Windows.Controls.TreeView)sender));
                    if (targetTreeViewItem != null && targetTreeViewItem.DataContext is FolderItem targetFolder)
                    {
                        // Don't allow dropping on self or parent
                        if (targetFolder.Path != draggedFolder.Path && 
                            !draggedFolder.Path.StartsWith(targetFolder.Path + "/", StringComparison.OrdinalIgnoreCase))
                        {
                            e.Effects = System.Windows.DragDropEffects.Move;
                            e.Handled = true;
                            return;
                        }
                    }
                }
                e.Effects = System.Windows.DragDropEffects.None;
                e.Handled = true;
                return;
            }

            // Check for file drop
            if (e.Data.GetDataPresent(System.Windows.DataFormats.FileDrop))
            {
                e.Effects = System.Windows.DragDropEffects.Copy;
                e.Handled = true;
            }
        }

        private async void OnFolderTreeDrop(object sender, System.Windows.DragEventArgs e)
        {
            if (_folderManagementService == null || _viewModel == null) return;

            // Get target folder
            var targetTreeViewItem = GetTreeViewItemUnderPoint((System.Windows.Controls.TreeView)sender, e.GetPosition((System.Windows.Controls.TreeView)sender));
            FolderItem? targetFolder = null;
            if (targetTreeViewItem != null && targetTreeViewItem.DataContext is FolderItem folder)
            {
                targetFolder = folder;
            }
            else
            {
                // Drop on root
                targetFolder = _viewModel.Folders.FirstOrDefault();
                if (targetFolder == null) return;
            }

            string targetPath = targetFolder.Path;

            // Handle resource drop
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                if (e.Data.GetData("ResourceBrowserItem") is ResourceBrowserItem resource)
                {
                    // Store the resource info before moving
                    int resourceId = resource.Id;
                    string oldVirtualPath = resource.VirtualPath;
                    
                    bool success = await _folderManagementService.MoveResourceAsync(resourceId, targetPath);
                    if (success)
                    {
                        // Only refresh resources list, not rebuild folder tree
                        await RefreshResourcesAfterMove(resourceId, oldVirtualPath);
                    }
                    else
                    {
                        System.Windows.MessageBox.Show("Failed to move resource", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                    }
                    e.Handled = true;
                    return;
                }
            }

            // Handle folder drop
            if (e.Data.GetDataPresent("FolderItem"))
            {
                if (e.Data.GetData("FolderItem") is FolderItem draggedFolder)
                {
                    string sourcePath = draggedFolder.Path;
                    bool success = await _folderManagementService.MoveFolderAsync(sourcePath, targetPath);
                    if (success)
                    {
                        // Refresh everything after folder move
                        await System.Windows.Application.Current.Dispatcher.InvokeAsync(() =>
                        {
                            // Save expanded paths before update
                            var expandedPaths = GetExpandedFolderPaths();
                            
                            _viewModel?.RefreshResources();
                            
                            // Restore expanded state
                            RestoreExpandedFolderPaths(expandedPaths);
                        });
                    }
                    else
                    {
                        System.Windows.MessageBox.Show("Failed to move folder", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                    }
                    e.Handled = true;
                    return;
                }
            }

            // Handle file drop
            if (e.Data.GetDataPresent(System.Windows.DataFormats.FileDrop) && _importService != null)
            {
                string[] files = (string[])e.Data.GetData(System.Windows.DataFormats.FileDrop);
                if (files != null && files.Length > 0)
                {
                    bool success = await _importService.ImportResourceFromDragDropAsync(files, targetPath);
                    if (success)
                    {
                        // Only refresh resources, folder tree will be updated when needed
                        await RefreshResourcesAfterImport();
                    }
                }
                e.Handled = true;
            }
        }

        // Folder tree item drag handlers
        private void OnFolderTreeItemMouseMove(object sender, System.Windows.Input.MouseEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed && !_isDragging)
            {
                if (sender is FrameworkElement element && element.DataContext is FolderItem item)
                {
                    System.Windows.Point currentPoint = e.GetPosition(null);
                    if (Math.Abs(currentPoint.X - _dragStartPoint.X) > SystemParameters.MinimumHorizontalDragDistance ||
                        Math.Abs(currentPoint.Y - _dragStartPoint.Y) > SystemParameters.MinimumVerticalDragDistance)
                    {
                        _draggedFolder = item;
                        _isDragging = true;
                        
                        System.Windows.DataObject dataObject = new System.Windows.DataObject("FolderItem", item);
                        DragDrop.DoDragDrop(element, dataObject, System.Windows.DragDropEffects.Move);
                        
                        _isDragging = false;
                        _draggedFolder = null;
                    }
                }
            }
        }

        private void OnFolderTreeItemMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _dragStartPoint = e.GetPosition(null);
        }

        // Folder tree context menu
        private void OnFolderTreeContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            var treeView = sender as System.Windows.Controls.TreeView;
            if (treeView == null) return;

            // Get the TreeViewItem under the cursor
            var hitTestResult = VisualTreeHelper.HitTest(treeView, Mouse.GetPosition(treeView));
            var treeViewItem = FindAncestor<TreeViewItem>(hitTestResult?.VisualHit);
            
            FolderItem? folder = null;
            if (treeViewItem != null && treeViewItem.DataContext is FolderItem folderItem)
            {
                folder = folderItem;
                treeViewItem.IsSelected = true;
            }
            else if (treeView.SelectedItem is FolderItem selectedFolder)
            {
                folder = selectedFolder;
            }
            else if (_viewModel?.Folders.Count > 0)
            {
                // Right-click on empty area, use root folder
                folder = _viewModel.Folders[0];
            }

            if (folder != null)
            {
                ContextMenu contextMenu = new ContextMenu();
                
                MenuItem createFolderItem = new MenuItem { Header = "Create Folder" };
                createFolderItem.Click += async (s, args) => 
                {
                    contextMenu.IsOpen = false;
                    await OnCreateFolder(folder.Path);
                };
                contextMenu.Items.Add(createFolderItem);
                
                // Only show rename/delete for non-root folders
                if (!string.IsNullOrEmpty(folder.Path))
                {
                    MenuItem renameFolderItem = new MenuItem { Header = "Rename Folder" };
                    renameFolderItem.Click += async (s, args) => 
                    {
                        contextMenu.IsOpen = false;
                        await OnRenameFolder(folder);
                    };
                    contextMenu.Items.Add(renameFolderItem);
                    
                    MenuItem deleteFolderItem = new MenuItem { Header = "Delete Folder" };
                    deleteFolderItem.Click += async (s, args) => 
                    {
                        contextMenu.IsOpen = false;
                        await OnDeleteFolder(folder);
                    };
                    contextMenu.Items.Add(deleteFolderItem);
                }
                
                treeView.ContextMenu = contextMenu;
            }
            else
            {
                e.Handled = true;
            }
        }

        private async System.Threading.Tasks.Task RefreshResourcesAfterMove(int resourceId, string oldVirtualPath)
        {
            if (_viewModel == null) return;

            // Refresh resources list without rebuilding folder tree
            await System.Windows.Application.Current.Dispatcher.InvokeAsync(() =>
            {
                _viewModel.RefreshResourcesOnly();
                
                // Try to select the moved resource
                var movedResource = _viewModel.Resources.FirstOrDefault(r => r.Id == resourceId);
                if (movedResource != null)
                {
                    _viewModel.SelectResource(movedResource);
                }
            });
        }

        private async System.Threading.Tasks.Task RefreshResourcesAfterImport()
        {
            if (_viewModel == null) return;

            // Refresh resources list and update folder tree if needed
            await System.Windows.Application.Current.Dispatcher.InvokeAsync(() =>
            {
                _viewModel.RefreshResourcesOnly();
                _viewModel.UpdateFolderTree();
            });
        }

        private async System.Threading.Tasks.Task OnCreateFolder(string parentPath)
        {
            if (_folderManagementService == null || _viewModel == null) return;

            var dialog = new Views.InputDialog("Enter folder name:", "Create Folder", "");
            if (dialog.ShowDialog() == true && !string.IsNullOrWhiteSpace(dialog.Answer))
            {
                string newFolderName = dialog.Answer.Trim();
                bool success = await _folderManagementService.CreateFolderAsync(parentPath, newFolderName);
                if (success)
                {
                    // Only update folder tree, resources don't change
                    await System.Windows.Application.Current.Dispatcher.InvokeAsync(() =>
                    {
                        // Save expanded paths before update
                        var expandedPaths = GetExpandedFolderPaths();
                        string newFolderPath = string.IsNullOrEmpty(parentPath) 
                            ? newFolderName 
                            : $"{parentPath}/{newFolderName}";
                        
                        _viewModel.UpdateFolderTree();
                        
                        // Restore expanded state and expand to new folder
                        RestoreExpandedFolderPaths(expandedPaths);
                        ExpandToFolder(newFolderPath);
                    });
                }
                else
                {
                    System.Windows.MessageBox.Show("Failed to create folder", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            }
        }

        private async System.Threading.Tasks.Task OnRenameFolder(FolderItem folder)
        {
            if (_folderManagementService == null || _viewModel == null) return;

            var dialog = new Views.InputDialog("Enter new folder name:", "Rename Folder", folder.Name);
            if (dialog.ShowDialog() == true && !string.IsNullOrWhiteSpace(dialog.Answer))
            {
                string oldPath = folder.Path;
                bool success = await _folderManagementService.RenameFolderAsync(folder.Path, dialog.Answer.Trim());
                if (success)
                {
                    // Refresh resources and folder tree after rename
                    await System.Windows.Application.Current.Dispatcher.InvokeAsync(() =>
                    {
                        // Save expanded paths before update
                        var expandedPaths = GetExpandedFolderPaths();
                        
                        _viewModel.RefreshResourcesOnly();
                        _viewModel.UpdateFolderTree();
                        
                        // Restore expanded state
                        RestoreExpandedFolderPaths(expandedPaths);
                    });
                }
                else
                {
                    System.Windows.MessageBox.Show("Failed to rename folder", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            }
        }

        private async System.Threading.Tasks.Task OnDeleteFolder(FolderItem folder)
        {
            if (_folderManagementService == null || _viewModel == null) return;

            var result = System.Windows.MessageBox.Show(
                $"Are you sure you want to delete folder '{folder.Name}' and all its contents?",
                "Delete Folder",
                System.Windows.MessageBoxButton.YesNo,
                System.Windows.MessageBoxImage.Warning);

            if (result == System.Windows.MessageBoxResult.Yes)
            {
                bool success = await _folderManagementService.DeleteFolderAsync(folder.Path);
                if (success)
                {
                    // Refresh everything after delete
                    await System.Windows.Application.Current.Dispatcher.InvokeAsync(() =>
                    {
                        // Save expanded paths before update (excluding deleted folder)
                        var expandedPaths = GetExpandedFolderPaths();
                        expandedPaths.RemoveWhere(p => p.StartsWith(folder.Path + "/", StringComparison.OrdinalIgnoreCase) || 
                                                       p.Equals(folder.Path, StringComparison.OrdinalIgnoreCase));
                        
                        _viewModel.RefreshResourcesOnly();
                        _viewModel.UpdateFolderTree();
                        
                        // Restore expanded state
                        RestoreExpandedFolderPaths(expandedPaths);
                    });
                }
                else
                {
                    System.Windows.MessageBox.Show("Failed to delete folder", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            }
        }

        private async void OnChangeVirtualPath(object sender, RoutedEventArgs e)
        {
            if (_viewModel?.SelectedResource == null || _folderManagementService == null) return;

            var dialog = new Views.InputDialog("Enter new virtual path (including filename):", "Change Virtual Path", _viewModel.SelectedResource.VirtualPath);
            if (dialog.ShowDialog() == true && !string.IsNullOrWhiteSpace(dialog.Answer))
            {
                // Extract parent path and resource name
                string newPath = dialog.Answer.Trim();
                
                // Validate path format
                if (!_folderManagementService.IsValidVirtualPath(newPath))
                {
                    System.Windows.MessageBox.Show("Invalid virtual path format. Path should not contain backslashes or start/end with slashes.", 
                        "Invalid Path", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
                    return;
                }

                string parentPath = "";
                string resourceName = newPath;

                int lastSlash = newPath.LastIndexOf('/');
                if (lastSlash >= 0)
                {
                    parentPath = newPath.Substring(0, lastSlash);
                    resourceName = newPath.Substring(lastSlash + 1);
                }

                // Update manifest directly to set the complete virtual path
                var project = ServiceLocator.Get<IProjectManager>().CurrentProject;
                if (project == null) return;

                string? packagePath = ProjectManager.GetPackagePath(project.ProjectPath);
                if (string.IsNullOrEmpty(packagePath)) return;

                string manifestPath = Path.Combine(packagePath, "resource_manifest.json");
                if (!File.Exists(manifestPath)) return;

                try
                {
                    string jsonContent = File.ReadAllText(manifestPath);
                    var manifest = Newtonsoft.Json.JsonConvert.DeserializeObject<Models.ResourceManifest>(jsonContent);
                    if (manifest?.Resources != null)
                    {
                        var resource = manifest.Resources.FirstOrDefault(r => r.Id == _viewModel.SelectedResource.Id);
                        if (resource != null)
                        {
                            resource.VirtualPath = newPath;
                            File.WriteAllText(manifestPath, 
                                Newtonsoft.Json.JsonConvert.SerializeObject(manifest, Newtonsoft.Json.Formatting.Indented));
                            _viewModel.RefreshResources();
                            System.Windows.MessageBox.Show("Virtual path updated successfully", "Success", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
                        }
                    }
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show($"Failed to change virtual path: {ex.Message}", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            }
        }

        // Helper methods
        private TreeViewItem? GetTreeViewItemUnderPoint(ItemsControl itemsControl, System.Windows.Point point)
        {
            var hitTestResult = VisualTreeHelper.HitTest(itemsControl, point);
            return FindAncestor<TreeViewItem>(hitTestResult?.VisualHit);
        }

        private static T? FindAncestor<T>(DependencyObject? current) where T : DependencyObject
        {
            while (current != null)
            {
                if (current is T ancestor)
                    return ancestor;
                current = VisualTreeHelper.GetParent(current);
            }
            return null;
        }

        /// <summary>
        /// 获取所有展开的文件夹路径
        /// </summary>
        private HashSet<string> GetExpandedFolderPaths()
        {
            var expandedPaths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (FolderTreeView == null) return expandedPaths;

            foreach (var item in FolderTreeView.Items)
            {
                if (item is FolderItem folder)
                {
                    CollectExpandedPaths(folder, expandedPaths);
                }
            }

            return expandedPaths;
        }

        /// <summary>
        /// 递归收集展开的文件夹路径
        /// </summary>
        private void CollectExpandedPaths(FolderItem folder, HashSet<string> expandedPaths)
        {
            if (folder == null) return;

            // Find the TreeViewItem for this folder
            var treeViewItem = FindTreeViewItem(FolderTreeView, folder);
            if (treeViewItem != null && treeViewItem.IsExpanded)
            {
                expandedPaths.Add(folder.Path);
            }

            // Recursively check children
            if (folder.Children != null)
            {
                foreach (var child in folder.Children)
                {
                    CollectExpandedPaths(child, expandedPaths);
                }
            }
        }

        /// <summary>
        /// 恢复展开的文件夹路径
        /// </summary>
        private void RestoreExpandedFolderPaths(HashSet<string> expandedPaths)
        {
            if (FolderTreeView == null || expandedPaths.Count == 0) return;

            foreach (var item in FolderTreeView.Items)
            {
                if (item is FolderItem folder)
                {
                    RestoreExpandedState(folder, expandedPaths);
                }
            }
        }

        /// <summary>
        /// 递归恢复展开状态
        /// </summary>
        private void RestoreExpandedState(FolderItem folder, HashSet<string> expandedPaths)
        {
            if (folder == null) return;

            // Find and expand the TreeViewItem if it was expanded before
            if (expandedPaths.Contains(folder.Path))
            {
                var treeViewItem = FindTreeViewItem(FolderTreeView, folder);
                if (treeViewItem != null)
                {
                    treeViewItem.IsExpanded = true;
                }
            }

            // Recursively restore children
            if (folder.Children != null)
            {
                foreach (var child in folder.Children)
                {
                    RestoreExpandedState(child, expandedPaths);
                }
            }
        }

        /// <summary>
        /// 展开到指定文件夹路径
        /// </summary>
        private void ExpandToFolder(string folderPath)
        {
            if (FolderTreeView == null || string.IsNullOrEmpty(folderPath)) return;

            // Use Dispatcher to ensure UI is updated
            Dispatcher.BeginInvoke(new System.Action(() =>
            {
                string[] parts = folderPath.Split('/');
                FolderItem? currentFolder = null;

                // Find the root folder
                foreach (var item in FolderTreeView.Items)
                {
                    if (item is FolderItem folder && string.IsNullOrEmpty(folder.Path))
                    {
                        currentFolder = folder;
                        break;
                    }
                }

                if (currentFolder == null) return;

                // Navigate through the path and expand each level
                foreach (string part in parts)
                {
                    if (string.IsNullOrEmpty(part)) continue;

                    var child = currentFolder.Children?.FirstOrDefault(c => c.Name == part);
                    if (child == null) break;

                    // Expand this level
                    var treeViewItem = FindTreeViewItem(FolderTreeView, child);
                    if (treeViewItem != null)
                    {
                        treeViewItem.IsExpanded = true;
                        // Update layout to ensure children are generated
                        treeViewItem.UpdateLayout();
                    }

                    currentFolder = child;
                }
            }), System.Windows.Threading.DispatcherPriority.Loaded);
        }

        /// <summary>
        /// 在TreeView中查找指定FolderItem对应的TreeViewItem
        /// </summary>
        private TreeViewItem? FindTreeViewItem(ItemsControl parent, FolderItem targetFolder)
        {
            if (parent == null || targetFolder == null) return null;

            // Wait for containers to be generated
            parent.UpdateLayout();

            foreach (var item in parent.Items)
            {
                if (item is FolderItem folder)
                {
                    var container = parent.ItemContainerGenerator.ContainerFromItem(item) as TreeViewItem;
                    if (container != null)
                    {
                        if (folder == targetFolder)
                        {
                            return container;
                        }

                        // Recursively search in children
                        if (folder.Children != null)
                        {
                            var found = FindTreeViewItem(container, targetFolder);
                            if (found != null) return found;
                        }
                    }
                }
            }

            return null;
        }
    }
}
