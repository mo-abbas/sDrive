using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.SceneManagement;

public class Menu : MonoBehaviour {

    public GameObject process_status;

    public void goToMainScene()
    {
        SceneManager.LoadScene("main");
    }

    [DllImport("sDrive-Core", CallingConvention = CallingConvention.Cdecl)]
    private extern static bool processVideos([MarshalAs(UnmanagedType.LPStr)]string directory);
    
    public void processVideos()
    {
        var myT = new System.Threading.Thread(new System.Threading.ThreadStart(doProcessing));
        myT.Start();
    }

    public void doProcessing()
    {
        process_status.SetActive(false);
        
        if (!processVideos(System.IO.Path.Combine(Application.dataPath, @"..\Input\")))
        {
            process_status.GetComponent<UnityEngine.UI.Text>().text = "An error occurred while processing the videos";
            process_status.GetComponent<UnityEngine.UI.Text>().color = Color.red;
        }
        else
        {
            process_status.GetComponent<UnityEngine.UI.Text>().text = "Video processing successful";
            process_status.GetComponent<UnityEngine.UI.Text>().color = Color.green;
        }

        process_status.SetActive(true);
    }

    public void doExit()
    {
#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
#endif
        Application.Quit();
    }
}
