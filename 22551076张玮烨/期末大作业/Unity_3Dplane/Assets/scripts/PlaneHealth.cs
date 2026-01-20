using UnityEngine;

public class PlaneHealth : MonoBehaviour
{
    [Header("Health")]
    public float maxHealth = 50f;
    public float currentHealth;

    void Start()
    {
        currentHealth = maxHealth;
    }

    public void TakeDamage(float damage)
    {
        currentHealth -= damage;
        currentHealth = Mathf.Clamp(currentHealth, 0f, maxHealth);

        Debug.Log($"Plane Hit! Damage: {damage}, Health: {currentHealth}");
        
    }


}