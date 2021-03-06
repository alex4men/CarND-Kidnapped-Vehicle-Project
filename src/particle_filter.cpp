/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
using std::normal_distribution;
using std::cout;
using std::endl;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * Set the number of particles. Initialize all particles to
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 100;  // Set the number of particles
  
  // Create random generator
  std::default_random_engine gen;
  
  // Create normal (Gaussian) distributions for x,y and theta
  normal_distribution<double> dist_x(x, std[0]);
  normal_distribution<double> dist_y(y, std[1]);
  normal_distribution<double> dist_theta(theta, std[2]);
  
  // Initialize particles around gps location with normal distribution with weight = 1
  for (int i = 0; i < num_particles; ++i) {
    particles.push_back(Particle{i, dist_x(gen), dist_y(gen), dist_theta(gen), 1});
  }
  
  // UNCOMMENT TO SEE THIS STEP OF THE FILTER
//  cout << "Initial particles: " << endl;
//  for (int i = 0; i < num_particles; ++i) {
//    cout << particles[i].id << "\t" << particles[i].x << "\t" << particles[i].y << "\t"
//         << particles[i].theta << "\t" << particles[i].weight << endl;
//  }
//  cout << "END of initialization" << endl;
  
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  // Create random generator
  std::default_random_engine gen;
  
  for (int i = 0; i < num_particles; ++i) {
    double x = particles[i].x;
    double y = particles[i].y;
    double theta = particles[i].theta;
    
    // predict particle's position using our motion model
    // avoid division by zero
    if (yaw_rate == 0) {
      x += velocity * cos(theta) * delta_t;
      y += velocity * sin(theta) * delta_t;
    } else {
      x += velocity * ( sin(theta + yaw_rate * delta_t) - sin(theta) ) / yaw_rate;
      y += velocity * ( -cos(theta + yaw_rate * delta_t) + cos(theta) ) / yaw_rate;
      theta += yaw_rate * delta_t;
    }
    
    // Create normal (Gaussian) distributions for x,y and theta
    normal_distribution<double> dist_x(x, std_pos[0]);
    normal_distribution<double> dist_y(y, std_pos[1]);
    normal_distribution<double> dist_theta(theta, std_pos[2]);
    
    // Add noize to the particle's movement
    particles[i].x = dist_x(gen);
    particles[i].y = dist_y(gen);
    particles[i].theta = dist_theta(gen);
  }
}

int ParticleFilter::dataAssociation(LandmarkObs observation, const Map &map_landmarks) {
  /**
   * Find the predicted measurement that is closest to the
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
  int closest_landmark_id = 0;
  int min_dist = 999999;
  double curr_dist;
  
  // Iterate through all landmarks to check which is closest
  for (int i = 0; i < map_landmarks.landmark_list.size(); ++i) {
    

    // Calculate Euclidean distance
    curr_dist = sqrt(pow(observation.x - map_landmarks.landmark_list[i].x_f, 2)
                     + pow(observation.y - map_landmarks.landmark_list[i].y_f, 2));
    
    // Compare to min_dist and update if it's closer
    if (curr_dist < min_dist) {
      min_dist = curr_dist;
      closest_landmark_id = i;
    }
  }
  return closest_landmark_id;
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * Update the weights of each particle using a mult-variate Gaussian
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
  // Reset max weight
  max_weight = 0;
  
  // For each particle transform observations to the map's coordinates
  for (auto &particle:particles) {
    particle.weight = 1;
    
    for (auto observation:observations) {
      LandmarkObs transformed_obs = transform_obs(particle.x, particle.y, particle.theta, observation);
      
      // Find out which landmark does it correspond to?
      int id = dataAssociation(transformed_obs, map_landmarks);
      
      // With what probability?
      double weight_part = normPdf2d(transformed_obs.x, transformed_obs.y,
                                     map_landmarks.landmark_list[id].x_f, map_landmarks.landmark_list[id].y_f,
                                     std_landmark[0], std_landmark[1]);
      
      // Accumulate the resulting weight
      particle.weight *= weight_part;
    }
    
    // update the maximum weight
    if (particle.weight > max_weight) {
      max_weight = particle.weight;
    }
  }
  
  // UNCOMMENT TO SEE THIS STEP OF THE FILTER
//    cout << "Update Weights: " << endl;
//    for (int k = 0; k < particles.size(); ++k) {
//      cout << k << "\t" << particles[k].x << "\t" << particles[k].y << "\t" << particles[k].weight << endl;
//    }
//    cout << "End of the update" << endl;
}

void ParticleFilter::resample() {
  /**
   * Resample particles with replacement with probability proportional
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  // Create random generator stuff
  std::default_random_engine gen;
  std::uniform_real_distribution<> rand_beta(0, max_weight);
  std::discrete_distribution<> rand_index(0, num_particles);
  std::vector<Particle> resampled_particles;
  
  
  int index = rand_index(gen);
  double b = 0;
  
  // Resampling wheel algorithm
  for (int i = 0; i < num_particles; ++i) {
    b += rand_beta(gen);
    
    while (b > particles[index].weight) {
      b = b - particles[index].weight;
      index = (index + 1) % num_particles;
    }
    resampled_particles.push_back(particles[index]);
  }
  
  particles = resampled_particles;
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}
