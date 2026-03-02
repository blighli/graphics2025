using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;
using System.Collections;
using TMPro;

public class GameOverManager : MonoBehaviour
{
    public GameObject gameOverPanel;
    public CanvasGroup canvasGroup;
    public TextMeshProUGUI distanceText;
    public TextMeshProUGUI bestText;

    public float fadeDuration = 0.6f;

    bool isGameOver = false;

    void Start()
    {
        gameOverPanel.SetActive(false);
        canvasGroup.alpha = 0f;
    }

    public void ShowGameOver(float distance)
    {
        if (isGameOver) return;
        isGameOver = true;

        gameOverPanel.SetActive(true);

        int dist = Mathf.FloorToInt(distance);
        distanceText.text = $"Flight Distance: {dist}";

        int best = PlayerPrefs.GetInt("BestDistance", 0);
        if (dist > best)
        {
            best = dist;
            PlayerPrefs.SetInt("BestDistance", best);
            distanceText.text = $"Flight Distance: {dist} (BEST)";
        }
        bestText.text = $"Best Flight Distance: {best}";

        StartCoroutine(FadeIn());
    }

    IEnumerator FadeIn()
    {
        float t = 0f;

        while (t < fadeDuration)
        {
            t += Time.unscaledDeltaTime;
            canvasGroup.alpha = Mathf.Lerp(0f, 1f, t / fadeDuration);
            yield return null;
        }

        canvasGroup.alpha = 1f;
        canvasGroup.interactable = true;
        canvasGroup.blocksRaycasts = true;

        // 最后再暂停游戏
        Time.timeScale = 0f;
    }

    public void Restart()
    {
        Time.timeScale = 1f;
        SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);
    }

    public void Quit()
    {
        Application.Quit();
    }
}