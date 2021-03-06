#include <ecn_visualodom/visual_odom.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using std::vector;
using std::cout;
using std::endl;
using namespace cv;

VisualOdom::VisualOdom(const vpCameraParameters cam, bool _relative_to_initial) :matcher(cv::NORM_HAMMING)
{
  // init calibration matrices from camera parameters
  Kcv = (cv::Mat1d(3, 3) <<
         cam.get_px(), 0, cam.get_u0(),
         0, cam.get_py(), cam.get_v0(),
         0, 0, 1);
  K = cam.get_K();
  Ki = cam.get_K_inverse();

  // no image yet
  first_time = true;
  relative_to_initial = _relative_to_initial;

  // matching
  akaze = cv::AKAZE::create();

  // default guesses
  n_guess.resize(3);
  n_guess[2] = 1;
}



// process a new image and extracts relative transform
bool VisualOdom::process(cv::Mat &im2, vpHomogeneousMatrix &_M)
{
  // to gray level
  // TO DO
  cv::Mat img;
  cvtColor(im2, img, cv::COLOR_RGB2GRAY);
  if(first_time)
  {
    // just detect and store point features  into kp1, des1 with akaze
    // TO DO
    akaze->detectAndCompute(img,cv::noArray(), kp1, des1);
    first_time = false;
    // copy this image
    im2.copyTo(im1);
    return false;
  }
  else
  {
    // detect point features into kp2, des2

    akaze->detectAndCompute(img,cv::noArray(), kp2, des2);
    // match with stored features

    matcher.match(des1, des2, matches);
    // build vectors of matched points
    std::vector<cv::Point2f> matched1, matched2;
    for(auto &m: matches)
    {
      matched1.push_back(kp1[m.queryIdx].pt);
      matched2.push_back(kp2[m.trainIdx].pt);
    }

    mask = cv::Mat(matches.size(), 1, cv::DataType<int>::type, cv::Scalar(1));

    // use RANSAC to compute homography and store it in Hp
    Hp = findHomography(matched1,matched2,mask,cv::RANSAC,5);

    // show correct matches
    cv::drawMatches(im1, kp1, im2, kp2, matches, imatches, cv::Scalar::all(-1), cv::Scalar::all(-1), mask);
    cv::imshow("Matches", imatches);

    // keep only inliers
    int mask_count = 0;
    for(unsigned i = 0; i < matched1.size(); i++)
    {
      if(mask.at<int>(i))
      {
        // write this index in the next valid element
        std::iter_swap(matched1.begin()+i, matched1.begin()+mask_count);
        std::iter_swap(matched2.begin()+i, matched2.begin()+mask_count);
        mask_count++;
      }
    }

    std::cout << "Matches: " << matches.size() << ", kept " << mask_count << std::endl;

    if(mask_count)
    {
      // resize vectors to mask size
      matched1.resize(mask_count);
      matched2.resize(mask_count);

      // decompose homography -> n solutions in (R,t,nor)
      int n = 0;
      n=cv::decomposeHomographyMat(Hp,Kcv,R,t,nor);
      // TO DO
      cout << " Found " << n << " solutions" << endl;
      // build corresponding homography candidates
      H.resize(n);
      for(unsigned int i=0;i<n;++i)
        H[i].buildFrom(R[i], t[i], nor[i]);

      // prune some homography candidates based on point being at negative Z
      // build normalized coordinates of points in camera 1
      vpMatrix X1 = cvPointToNormalized(matched1); // dim(X1) = (3, nb_pts)

      for(unsigned int j=0;j<matched1.size();++j)
      {
        for(unsigned int i=0;i<H.size();++i)
        {
          // compute Z from X.nx + Y.ny + Z.nz = d
          // equivalent to x.nx + y.ny + nz = d/Z
          // hence sign(Z) = x.nx + y.ny + nz (positive d)
          // if Z is negative then this transform is not possible

          if(H[i].n.t()*X1.getCol(j)<0)
          {
            cout << "discarded solution, negative depth" << endl;
            H.erase(H.begin()+i);
            break;
          }
        }
      }

      // if all pruned, wrong behavior, exit
      if(H.size() == 0)
        return false;

      // assume best solution is H[0]
      int idx = 0;

      // if 2 solutions left, check against normal guess
      if(H.size() == 2)
      {
        // compare normals H[0].n and H[1].n to current normal estimation n_guess
        // change idx to 1 if needed
        // TO DO
          auto prod1 = H[0].n.t()*n_guess ;
          auto prod2 = H[1].n.t()*n_guess ;
          //if (abs(prod2)-1 < abs(prod1)-1) {
          if (prod2 * prod2 > prod1 * prod1) {
              idx = 1;
          }
          cout << "Best solution found - "<<idx << endl;
      }
      if(H.size() == 1)
      {
          cout << "Best solution found" << endl;
      }


      // rescale translation from scale guess
      if(d_guess == 0)    // first guess from t0 and normal
      {
         d_guess = H[idx].n.t()*t0;

      }
      H[idx].t=H[idx].t*d_guess;

      // build corresponding relative transformation
      _M.buildFrom(H[idx].t, H[idx].R);

      if(relative_to_initial)
      {
        // do not update descriptors, just refine normal
        // actual normal
        n_guess = H[idx].n;
      }
      else
      {
        // update reference image, keypoints and descriptors
        im2.copyTo(im1);
        kp1 = kp2;
        des1 = des2;

        // update estimation of normal in new frame
        n_guess =  H[idx].R*H[idx].n;

        // update plane distance in new frame
        // compute this value for all points and take the mean value
        vpColVector X2;
        vpRowVector d(X1.getCols());
        double Z1;
        vpColVector X;
        for(unsigned int i=0;i<X1.getCols();++i)
        {
          // Z1 from current distance guess
          // TO DO
          Z1 = d_guess/(H[idx].n.t()*X1.getCol(i));
          auto X = Z1 * X1.getCol(i);
          //  Coordinates of X in frame of camera 2
          // TO DO
          X2 = H[idx].R * X + H[idx].t;
          // corresponding distance d in camera 2
          // TO DO
          d[i] = n_guess.t()*X2;
        }
        // take the mean
        d_guess = vpRowVector::mean(d);
      }
    }
  }
  return true;

}
