using System;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Zpd.Editor
{
    public static class ProjectValidation
    {
        public static void Run()
        {
            int checkedScenes = 0;
            foreach (string guid in AssetDatabase.FindAssets("t:Scene", new[] { "Assets/Scenes" }))
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                var scene = EditorSceneManager.OpenScene(path, OpenSceneMode.Single);
                foreach (var root in scene.GetRootGameObjects())
                {
                    foreach (var child in root.GetComponentsInChildren<Transform>(true))
                    {
                        if (GameObjectUtility.GetMonoBehavioursWithMissingScriptCount(child.gameObject) != 0)
                            throw new InvalidOperationException("Missing script in " + path + ": " + child.name);
                    }
                }
                checkedScenes++;
            }
            if (checkedScenes == 0)
                throw new InvalidOperationException("No gameplay scenes were found.");
            Debug.Log("ZPD_VALIDATION_PASSED: compiled project and checked " + checkedScenes + " scenes.");
        }
    }
}
