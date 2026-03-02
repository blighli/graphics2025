using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class ScoreManager : MonoBehaviour
{
    public Transform plane;   // 飞机
    public TextMeshProUGUI scoreText;    // UI Text
    public PlaneHealth planeHealth;

    float startZ;
    int score;

    void Start()
    {
        if (plane != null)
            startZ = plane.position.z;
    }

    void Update()
    {
        if (plane == null) return;

        // 飞行距离 = 当前Z - 起始Z
        float distance = plane.position.z - startZ;

        score = Mathf.Max(0, Mathf.FloorToInt(distance));

        if (scoreText != null)
            scoreText.text = "Flight Distance\n" + score + "\n" ;
    }
}