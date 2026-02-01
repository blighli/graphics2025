using UnityEngine;

public class CameraShake : MonoBehaviour
{
    Vector3 originalPos;
    float shakeDuration = 0;
    float shakeStrength;

    void Start()
    {
        originalPos = transform.localPosition;
    }

    void LateUpdate()
    {
        
        if (shakeDuration > 0f)
        {
            transform.localPosition = originalPos + Random.insideUnitSphere * shakeStrength;
            shakeDuration -= Time.deltaTime;
        }
        else
        {
            shakeDuration = 0f;
            transform.localPosition = originalPos;
        }
    }

    public void Shake(float duration, float strength)
    {
        shakeDuration = duration;
        shakeStrength = strength;
    }
}