using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using TMPro;

public class PlayerStatusUI : MonoBehaviour
{
    [Header("UI References")]
    public Image backgroundImage;
    public TextMeshProUGUI statusText;

    [Header("Health")]
    public float maxHP = 50f;
    float currentHP;

    [Header("Panel Colors")]
    public Color normalColor = Color.white;
    public Color lowHPColor = new Color(1f, 0.3f, 0.3f);
    public Color hitFlashColor = Color.red;

    [Header("Text Colors")]
    public Color textHealthyColor = Color.green;
    public Color textLowHPColor = Color.red;

    [Header("Hit Flash")]
    public float flashDuration = 0.15f;

    Coroutine flashCoroutine;

    void Start()
    {
        currentHP = maxHP;
        UpdateUI();
    }


    public void TakeDamage(float damage)
    {
        currentHP -= damage;
        currentHP = Mathf.Clamp(currentHP, 0f, maxHP);

        UpdateUI();
        PlayHitFlash();
        
        if (currentHP <= 0)
        {
            PlaneController plane = FindObjectOfType<PlaneController>();
            if (plane != null)
                plane.Explode();
            
        }
    }

    void UpdateUI()
    {
        float hpPercent = currentHP / maxHP;

        // —— 面板颜色（随血量变红）——
        Color panelColor = Color.Lerp(lowHPColor, normalColor, hpPercent);
        backgroundImage.color = panelColor;

        // —— 文本内容 ——
        int percent = Mathf.RoundToInt(hpPercent * 100f);
        statusText.text = $"Integrity: {percent}%";

        // —— 文本颜色（绿 → 红）——
        Color textColor = Color.Lerp(textLowHPColor, textHealthyColor, hpPercent);
        statusText.color = textColor;
    }

    void PlayHitFlash()
    {
        if (flashCoroutine != null)
            StopCoroutine(flashCoroutine);

        flashCoroutine = StartCoroutine(HitFlashCoroutine());
    }

    IEnumerator HitFlashCoroutine()
    {
        Color originalColor = backgroundImage.color;

        backgroundImage.color = hitFlashColor;
        yield return new WaitForSeconds(flashDuration);

        backgroundImage.color = originalColor;
    }
}