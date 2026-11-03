import json
import math
import os
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

def load_match_list():
    matches = []
    for fpath in ['data/wc2022_matches.json', 'data/wc2018_matches.json']:
        if os.path.exists(fpath):
            with open(fpath, 'r', encoding='utf-8') as f:
                data = json.load(f)
                matches.extend(data)
    print(f"Total available matches in index: {len(matches)}")
    return matches

def fetch_match_events(match_id):
    cache_dir = "data/events_cache"
    os.makedirs(cache_dir, exist_ok=True)
    cache_file = os.path.join(cache_dir, f"{match_id}.json")
    
    if os.path.exists(cache_file) and os.path.getsize(cache_file) > 1000:
        with open(cache_file, 'r', encoding='utf-8') as f:
            return json.load(f)
            
    url = f"https://raw.githubusercontent.com/statsbomb/open-data/master/data/events/{match_id}.json"
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            content = resp.read().decode('utf-8')
            with open(cache_file, 'w', encoding='utf-8') as f:
                f.write(content)
            return json.loads(content)
    except Exception as e:
        print(f"Error fetching match {match_id}: {e}")
        return None

def sb_to_sim_coords(x_sb, y_sb):
    # StatsBomb: 120 x 80 -> Sim: 105 x 68
    x_sim = x_sb * (105.0 / 120.0)
    y_sim = y_sb * (68.0 / 80.0)
    return x_sim, y_sim

def get_zone_18(x_sim, y_sim):
    # 6 longitudinal zones (0..5), 3 lateral zones (0..2)
    # Zone 0..17: zx + 6 * zy
    zx = int(x_sim / (105.0 / 6.0))
    if zx < 0: zx = 0
    if zx > 5: zx = 5
    
    zy = int(y_sim / (68.0 / 3.0))
    if zy < 0: zy = 0
    if zy > 2: zy = 2
    
    return zx + 6 * zy

def main():
    matches = load_match_list()
    target_matches = matches[:105] # At least 100 matches!
    print(f"Targeting {len(target_matches)} real professional international matches for training...")
    
    all_match_ids = [m['match_id'] for m in target_matches]
    
    # Download with thread pool
    print("Downloading / loading events in parallel...")
    events_by_match = {}
    with ThreadPoolExecutor(max_workers=10) as executor:
        future_to_id = {executor.submit(fetch_match_events, mid): mid for mid in all_match_ids}
        completed = 0
        for future in as_completed(future_to_id):
            mid = future_to_id[future]
            data = future.result()
            if data:
                events_by_match[mid] = data
            completed += 1
            if completed % 20 == 0 or completed == len(all_match_ids):
                print(f"Progress: {completed}/{len(all_match_ids)} matches loaded.")
                
    valid_matches_count = len(events_by_match)
    print(f"Successfully loaded and parsed {valid_matches_count} complete matches.")
    
    # Aggregation data structures
    pass_dist_bins = [0, 10, 20, 30, 40, 100] # meters
    pass_total_by_bin = [0] * (len(pass_dist_bins) - 1)
    pass_completed_by_bin = [0] * (len(pass_dist_bins) - 1)
    
    pass_pressured_total = 0
    pass_pressured_completed = 0
    pass_open_total = 0
    pass_open_completed = 0
    
    pass_directions = {"forward": 0, "sideways": 0, "backward": 0}
    
    shot_dist_bins = [0, 8, 14, 20, 26, 35, 100]
    shot_total_by_bin = [0] * (len(shot_dist_bins) - 1)
    shot_goals_by_bin = [0] * (len(shot_dist_bins) - 1)
    shot_xg_sum_by_bin = [0.0] * (len(shot_dist_bins) - 1)
    
    shot_outcomes = {"Goal": 0, "Saved": 0, "Blocked": 0, "Off Target": 0, "Post": 0, "Wayward": 0}
    shot_distances = []
    
    # 18-Zone Transition matrix: 18x18
    zone_transitions = [[0 for _ in range(18)] for _ in range(18)]
    
    total_passes = 0
    total_shots = 0
    
    for mid, events in events_by_match.items():
        prev_zone = -1
        prev_team_id = -1
        
        for ev in events:
            ev_type = ev.get('type', {}).get('name')
            team_id = ev.get('team', {}).get('id')
            location = ev.get('location')
            
            # Spatial transition tracking
            if location and len(location) >= 2:
                x_sim, y_sim = sb_to_sim_coords(location[0], location[1])
                curr_zone = get_zone_18(x_sim, y_sim)
                if prev_team_id == team_id and prev_zone >= 0 and prev_zone < 18:
                    zone_transitions[prev_zone][curr_zone] += 1
                prev_zone = curr_zone
                prev_team_id = team_id
            else:
                prev_zone = -1
                prev_team_id = -1
                
            if ev_type == 'Pass':
                total_passes += 1
                pass_info = ev.get('pass', {})
                length_m = pass_info.get('length', 0.0) * 0.9144 # yards to meters
                angle_rad = pass_info.get('angle', 0.0)
                outcome = pass_info.get('outcome') # None = completed
                is_completed = (outcome is None)
                is_pressured = ev.get('under_pressure', False)
                
                # Binning distance
                for b in range(len(pass_dist_bins) - 1):
                    if pass_dist_bins[b] <= length_m < pass_dist_bins[b+1]:
                        pass_total_by_bin[b] += 1
                        if is_completed:
                            pass_completed_by_bin[b] += 1
                        break
                        
                # Pressure
                if is_pressured:
                    pass_pressured_total += 1
                    if is_completed: pass_pressured_completed += 1
                else:
                    pass_open_total += 1
                    if is_completed: pass_open_completed += 1
                    
                # Angle direction: angle 0 is towards opponent goal
                abs_ang = abs(angle_rad)
                if abs_ang < 0.785: # within 45 degrees
                    pass_directions["forward"] += 1
                elif abs_ang < 2.356: # 45 to 135 degrees
                    pass_directions["sideways"] += 1
                else: # > 135 degrees
                    pass_directions["backward"] += 1
                    
            elif ev_type == 'Shot':
                total_shots += 1
                shot_info = ev.get('shot', {})
                loc = ev.get('location', [120, 40])
                # Goal center in StatsBomb is at (120, 40)
                dx_m = (120.0 - loc[0]) * (105.0 / 120.0)
                dy_m = (40.0 - loc[1]) * (68.0 / 80.0)
                dist_to_goal = math.sqrt(dx_m*dx_m + dy_m*dy_m)
                shot_distances.append(dist_to_goal)
                
                xg = shot_info.get('statsbomb_xg', 0.05)
                outcome_name = shot_info.get('outcome', {}).get('name', 'Off Target')
                is_goal = (outcome_name == 'Goal')
                
                if outcome_name in shot_outcomes:
                    shot_outcomes[outcome_name] += 1
                elif "Off" in outcome_name:
                    shot_outcomes["Off Target"] += 1
                else:
                    shot_outcomes["Off Target"] += 1
                    
                for b in range(len(shot_dist_bins) - 1):
                    if shot_dist_bins[b] <= dist_to_goal < shot_dist_bins[b+1]:
                        shot_total_by_bin[b] += 1
                        shot_xg_sum_by_bin[b] += xg
                        if is_goal:
                            shot_goals_by_bin[b] += 1
                        break

    # Normalize zone transition matrix
    normalized_transition = []
    for r in range(18):
        row_sum = sum(zone_transitions[r])
        if row_sum > 0:
            normalized_transition.append([count / row_sum for count in zone_transitions[r]])
        else:
            uniform = 1.0 / 18.0
            normalized_transition.append([uniform for _ in range(18)])

    # Pass completion rates
    pass_completion_by_dist = []
    for b in range(len(pass_dist_bins) - 1):
        rate = (pass_completed_by_bin[b] / pass_total_by_bin[b]) if pass_total_by_bin[b] > 0 else 0.8
        pass_completion_by_dist.append({
            "min_dist": pass_dist_bins[b],
            "max_dist": pass_dist_bins[b+1],
            "total": pass_total_by_bin[b],
            "completed": pass_completed_by_bin[b],
            "rate": round(rate, 4)
        })

    # Shot distance statistics
    shot_dist_mean = sum(shot_distances) / len(shot_distances) if shot_distances else 16.5
    shot_dist_var = sum((d - shot_dist_mean)**2 for d in shot_distances) / len(shot_distances) if shot_distances else 36.0
    shot_dist_std = math.sqrt(shot_dist_var)

    # Shot xG and conversion by distance
    shot_stats_by_dist = []
    for b in range(len(shot_dist_bins) - 1):
        n = shot_total_by_bin[b]
        avg_xg = (shot_xg_sum_by_bin[b] / n) if n > 0 else 0.05
        conv_rate = (shot_goals_by_bin[b] / n) if n > 0 else 0.05
        shot_stats_by_dist.append({
            "min_dist": shot_dist_bins[b],
            "max_dist": shot_dist_bins[b+1],
            "total_shots": n,
            "goals": shot_goals_by_bin[b],
            "avg_xg": round(avg_xg, 4),
            "conversion_rate": round(conv_rate, 4)
        })

    summary = {
        "num_matches_trained": valid_matches_count,
        "total_passes_analyzed": total_passes,
        "total_shots_analyzed": total_shots,
        "pass_completion_by_distance": pass_completion_by_dist,
        "pass_pressure_completion": {
            "pressured_rate": round(pass_pressured_completed / pass_pressured_total, 4) if pass_pressured_total > 0 else 0.72,
            "unpressured_rate": round(pass_open_completed / pass_open_total, 4) if pass_open_total > 0 else 0.86,
            "pressured_total": pass_pressured_total,
            "unpressured_total": pass_open_total
        },
        "pass_direction_distribution": {
            k: round(v / total_passes, 4) for k, v in pass_directions.items()
        },
        "shot_distance_distribution": {
            "mean_m": round(shot_dist_mean, 2),
            "std_m": round(shot_dist_std, 2),
            "total_shots": len(shot_distances)
        },
        "shot_outcome_distribution": {
            k: round(v / total_shots, 4) for k, v in shot_outcomes.items()
        },
        "shot_stats_by_distance": shot_stats_by_dist,
        "zone_18_transition_matrix": normalized_transition
    }

    with open("data/trained_empirical_model.json", "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)

    print("\n================ TRAINED MODEL RESULTS ================")
    print(f"Matches analyzed: {valid_matches_count}")
    print(f"Passes analyzed: {total_passes}")
    print(f"Shots analyzed: {total_shots}")
    print(f"Pass completion by distance: {pass_completion_by_dist}")
    print(f"Pass completion under pressure: {summary['pass_pressure_completion']}")
    print(f"Shot mean dist: {shot_dist_mean:.2f}m +/- {shot_dist_std:.2f}m")
    print(f"Shot outcome distribution: {summary['shot_outcome_distribution']}")
    print("Saved empirical data to data/trained_empirical_model.json")

if __name__ == "__main__":
    main()
