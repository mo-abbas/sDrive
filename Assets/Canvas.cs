using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Video;

public class Canvas : MonoBehaviour {

    public GameObject camera2d;
    public GameObject camera3d;

    public GameObject vFront;
    public GameObject vRight;
    public GameObject vBack;
    public GameObject vLeft;

    public Main mainModule;

    enum View
    {
        ORIGINAL = 0,
        DISPARITY = 1,
        ROAD = 2,
        BOX = 3,
        SEGMENT = 4
    };

    static bool is3dView = false;
    static List<Tuple<VideoPlayer, View>> videoPlayers = new List<Tuple<VideoPlayer, View>>();
    
    void Start()
    {
        videoPlayers.Clear();
        prepareAllVideos();
    }

    void Update()
    {
        if (Input.GetKey(KeyCode.Backspace))
            UnityEngine.SceneManagement.SceneManager.LoadScene("menu");
    }

    public void toggle2d3dView(GameObject button)
    {
        if (is3dView)
        {
            camera3d.SetActive(false);
            camera2d.SetActive(true);
            button.GetComponentInChildren<Text>().text = "3D";
            is3dView = false;
        }
        else
        {
            camera3d.transform.rotation = Quaternion.Euler(40, 0, 0);
            camera2d.SetActive(false);
            camera3d.SetActive(true);
            button.GetComponentInChildren<Text>().text = "2D";
            is3dView = true;
        }
    }

    void prepareAllVideos()
    {
        StartCoroutine(prepareVideos(new string[]{
                System.IO.Path.Combine(Application.dataPath, @"..\Input\front_left.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Input\right_left.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Input\back_left.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Input\left_left.mp4")
            }, View.ORIGINAL));

        StartCoroutine(prepareVideos(new string[]{
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Disparity\disp_front.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Disparity\disp_right.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Disparity\disp_back.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Disparity\disp_left.mp4")
            }, View.DISPARITY));

        StartCoroutine(prepareVideos(new string[]{
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Road\road_front.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Road\road_right.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Road\road_back.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Road\road_left.mp4")
            }, View.ROAD));

        StartCoroutine(prepareVideos(new string[]{
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Boxes\box_front.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Boxes\box_right.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Boxes\box_back.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Boxes\box_left.mp4")
            }, View.BOX));

        StartCoroutine(prepareVideos(new string[]{
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Segments\seg_front.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Segments\seg_right.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Segments\seg_back.mp4"),
                System.IO.Path.Combine(Application.dataPath, @"..\Output\Segments\seg_left.mp4")
            }, View.SEGMENT, true));
    }

    IEnumerator prepareVideos(string[] videos, View view, bool final = false)
    {
        for (int i = 0; i < videos.Length; i++)
        {
            if (!System.IO.File.Exists(videos[i]))
                continue;

            videoPlayers.Add(new Tuple<VideoPlayer, View>(gameObject.AddComponent<VideoPlayer>(), view));
            videoPlayers.Last().First.playOnAwake = false;
            videoPlayers.Last().First.isLooping = false;
            videoPlayers.Last().First.source = VideoSource.Url;
            videoPlayers.Last().First.url = videos[i];
            videoPlayers.Last().First.Prepare();
            
            ////Wait until video is prepared
            WaitForSeconds waitTime = new WaitForSeconds(1);
            while (!videoPlayers.Last().First.isPrepared)
            {
                yield return waitTime;
                break;
            }
        }

        if (final)
        {
            onDifferentView(0);
            mainModule.rendering_started += MainModule_rendering_started;
            mainModule.startRendering();            
        }
    }

    private void MainModule_rendering_started(object sender, System.EventArgs e)
    {
        playAll();
    }

    public void playAll()
    {
        foreach (var videoPlayer in videoPlayers)
            videoPlayer.First.Play();
    }

    public void pauseAll()
    {
        foreach (var videoPlayer in videoPlayers)
            videoPlayer.First.Pause();
    }

    public void onDifferentView(int value)
    {
        var list = videoPlayers.FindAll(t => t.Second ==  (View)value);

        vFront.GetComponent<RawImage>().texture = null;
        vRight.GetComponent<RawImage>().texture = null;
        vBack.GetComponent<RawImage>().texture = null;
        vLeft.GetComponent<RawImage>().texture = null;

        vFront.SetActive(false);
        vRight.SetActive(false);
        vBack.SetActive(false);
        vLeft.SetActive(false);

        for (int i = 0; i < list.Count; i++)
        {
            if (i % 4 == 0)         //FRONT
            {
                vFront.GetComponent<RawImage>().texture = list[i].First.texture;
                vFront.SetActive(true);
            }
            else if (i % 4 == 1)    //RIGHT
            {
                vRight.GetComponent<RawImage>().texture = list[i].First.texture;
                vRight.SetActive(true);
            }
            else if (i % 4 == 2)    //BACK
            {
                vBack.GetComponent<RawImage>().texture = list[i].First.texture;
                vBack.SetActive(true);
            }
            else if (i % 4 == 3)    //LEFT
            {
                vLeft.GetComponent<RawImage>().texture = list[i].First.texture;
                vLeft.SetActive(true);
            }
        }        
    }
}
