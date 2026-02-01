using UnityEngine;

public class CameraFollow : MonoBehaviour
{
    public Transform target;          // 飞机
    public Vector3 offset = new Vector3(0f, 3f, -8f); // 相机相对飞机的偏移
    public float smoothSpeed = 5f;    // 平滑速度

    void LateUpdate()
    {
        if (target == null) return;

        Vector3 desiredPos = target.position + offset;

        transform.position = Vector3.Lerp(
            transform.position,
            desiredPos,
            smoothSpeed * Time.deltaTime
        );
    }
}