using UnityEngine;
using UnityEngine.SceneManagement;
using TMPro;

public class MainMenuManager : MonoBehaviour
{
    [Header("UI")]
    public TextMeshProUGUI bestDistanceText;

    [Header("Scene")]
    public string gameSceneName = "SampleScene";

    void Start()
    {
        ShowBestDistance();
    }

    void ShowBestDistance()
    {
        int best = PlayerPrefs.GetInt("BestDistance", 0);
        if (best == 0)
        {
            bestDistanceText.text = "No Play Record";
        }
        else
        {
            bestDistanceText.text = $"Best Flight Distance: {best}";
        }
        
        
    }

    // ▶ Start Button
    public void StartGame()
    {
        SceneManager.LoadScene(gameSceneName);
    }

    // ❌ Quit Button
    public void QuitGame()
    {
        Application.Quit();
    }
}