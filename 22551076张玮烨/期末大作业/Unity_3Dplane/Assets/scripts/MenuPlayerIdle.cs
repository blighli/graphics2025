using UnityEngine;

public class MenuPlayerIdle : MonoBehaviour
{
    [Header("Position Float")]
    public float floatYAmplitude = 0.2f;   // 上下浮动幅度
    public float floatZAmplitude = 0.15f;  // 前后浮动幅度
    public float floatSpeed = 1.2f;

    [Header("Rotation")]
    public float rollAmplitude = 8f;       // 左右倾斜角度
    public float pitchAmplitude = 4f;      // 轻微前后点头
    public float rotateSpeed = 0.8f;

    Vector3 startPos;
    Quaternion startRot;
    float seed;

    void Start()
    {
        startPos = transform.position;
        startRot = transform.rotation;
        
        seed = Random.Range(0f, 100f);
    }

    void Update()
    {
        float t = Time.time + seed;

        // -------- 位置浮动 --------
        float offsetY = Mathf.Sin(t * floatSpeed) * floatYAmplitude;
        float offsetZ = Mathf.Cos(t * floatSpeed * 0.8f) * floatZAmplitude;

        transform.position = startPos + new Vector3(0f, offsetY, offsetZ);

        // -------- 姿态浮动 --------
        float roll  = Mathf.Sin(t * rotateSpeed) * rollAmplitude;
        float pitch = Mathf.Cos(t * rotateSpeed * 0.9f) * pitchAmplitude;

        Quaternion targetRot = Quaternion.Euler(
            pitch,
            0f,
            roll
        );

        transform.rotation = startRot * targetRot;
    }
}