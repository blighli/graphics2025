using UnityEngine;

public class AsteroidController : MonoBehaviour
{
    
    public PlayerStatusUI statusUI;
    [Header("Rotation")]
    public float minRotateSpeed = 20f;
    public float maxRotateSpeed = 80f;

    [Header("Destroy")]
    public float destroyDistance = 15f;
    public float destroyAfterHit = 3f;

    [Header("Hit Reaction")]
    public float hitForce = 14f;
    public float torqueForce = 8f;

    [Header("VFX")]
    public GameObject hitEffect;

    Vector3 rotateAxis;
    float rotateSpeed;
    Transform plane;

    Rigidbody rb;
    Collider col;

    bool hasBeenHit = false;

    void Start()
    {
        // 随机旋转参数
        rotateAxis = Random.onUnitSphere;
        rotateSpeed = Random.Range(minRotateSpeed, maxRotateSpeed);

        rb = GetComponent<Rigidbody>();
        col = GetComponent<Collider>();
        statusUI = FindObjectOfType<PlayerStatusUI>();
        // 找到飞机
        GameObject planeObj = GameObject.FindGameObjectWithTag("Player");
        if (planeObj != null)
        {
            plane = planeObj.transform;
        }
    }

    void Update()
    {
        RotateSelf();
        CheckDestroy();
    }

    void RotateSelf()
    {
        transform.Rotate(rotateAxis, rotateSpeed * Time.deltaTime, Space.Self);
    }

    void CheckDestroy()
    {
        if (plane == null || hasBeenHit) return;

        // 飞机飞过后，陨石销毁
        if (transform.position.z < plane.position.z - destroyDistance)
        {
            Destroy(gameObject);
        }
    }

    void OnCollisionEnter(Collision collision)
    {
        if (hasBeenHit) return;

        if (!collision.gameObject.CompareTag("Player")) return;
        
        // ---------- 正面撞击判定 ----------
        Vector3 hitDir = (transform.position - collision.transform.position).normalized;
        Vector3 planeForward = collision.transform.forward;

        // 点积 > 0，说明在飞机前方
        if (Vector3.Dot(hitDir, planeForward) < 0f)
            return;

        // ---------- 标记已撞 ----------
        hasBeenHit = true;
        
        PlaneHealth health = collision.gameObject.GetComponent<PlaneHealth>();
        if (health != null)
        {
            health.TakeDamage(rb.mass);
            statusUI.TakeDamage(rb.mass);
        }
        // ---------- 镜头震动 ----------
        CameraShake shake = Camera.main.GetComponent<CameraShake>();
        if (shake != null)
        {
            float strength = Mathf.Clamp(rb.mass * 0.15f, 0.2f, 0.6f);
            shake.Shake(0.2f, strength);
        }


        // ---------- 撞击特效 ----------
        if (hitEffect != null && collision.contacts.Length > 0)
        {
            Instantiate(hitEffect, collision.contacts[0].point, Quaternion.identity);
        }
        PlaneController planeC =
            collision.gameObject.GetComponent<PlaneController>();
        planeC.OnDamaged();
        // ---------- 不再危险 ----------
        gameObject.tag = "Untagged";
        col.isTrigger = true;

        // ---------- 撞飞 ----------
        rb.AddForce(hitDir * hitForce, ForceMode.Impulse);
        rb.AddTorque(Random.insideUnitSphere * torqueForce, ForceMode.Impulse);

        Destroy(gameObject, destroyAfterHit);
    }
}
