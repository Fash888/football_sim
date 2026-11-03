import json
import csv
import math
import statistics

def analyze_statsbomb(filepath):
    print("=== STATSBOMB REAL DATA ANALYSIS (Champions League Final 2019) ===")
    with open(filepath, 'r', encoding='utf-8') as f:
        events = json.load(f)

    passes = []
    completed_passes = []
    shots = []
    pass_lengths = []
    pass_speeds = []
    pass_durations = []
    shot_distances = []
    shot_angles = []

    # Pitch is 120 x 80 in StatsBomb yards/coords, normalize or convert to meters (105 x 68)
    scale_x = 105.0 / 120.0
    scale_y = 68.0 / 80.0

    for ev in events:
        ev_type = ev.get('type', {}).get('name')
        if ev_type == 'Pass':
            p_info = ev.get('pass', {})
            length_m = p_info.get('length', 0.0) * 0.9144 # yards to m
            duration = ev.get('duration', 0.0)
            outcome = p_info.get('outcome', {}).get('name', 'Complete') # if no outcome field, it's Complete
            is_complete = (outcome == 'Complete')

            pass_lengths.append(length_m)
            if duration > 0.1:
                pass_speeds.append(length_m / duration)
                pass_durations.append(duration)

            passes.append({
                'team': ev.get('team', {}).get('name'),
                'length': length_m,
                'duration': duration,
                'complete': is_complete,
                'angle': p_info.get('angle', 0.0),
                'height': p_info.get('height', {}).get('name')
            })
            if is_complete:
                completed_passes.append(length_m)

        elif ev_type == 'Shot':
            s_info = ev.get('shot', {})
            loc = ev.get('location', [120, 40])
            loc_x = loc[0] * scale_x
            loc_y = loc[1] * scale_y
            goal_x = 105.0
            goal_y = 34.0
            dist = math.sqrt((goal_x - loc_x)**2 + (goal_y - loc_y)**2)
            shot_distances.append(dist)
            outcome = s_info.get('outcome', {}).get('name', '')
            is_on_target = outcome in ['Goal', 'Saved', 'Saved to Post', 'Saved Off Target']
            shots.append({
                'team': ev.get('team', {}).get('name'),
                'dist': dist,
                'xg': s_info.get('statsbomb_xg', 0.0),
                'outcome': outcome,
                'on_target': is_on_target
            })

    print(f"Total Passes Attempted: {len(passes)}")
    print(f"Total Passes Completed: {len(completed_passes)} ({len(completed_passes)/len(passes)*100:.1f}%)")
    print(f"Pass Length (m): Mean={statistics.mean(pass_lengths):.2f}, Median={statistics.median(pass_lengths):.2f}, Min={min(pass_lengths):.2f}, Max={max(pass_lengths):.2f}")
    if pass_speeds:
        print(f"Pass Speed (m/s): Mean={statistics.mean(pass_speeds):.2f}, Median={statistics.median(pass_speeds):.2f}, 10th%={statistics.quantiles(pass_speeds, n=10)[0]:.2f}, 90th%={statistics.quantiles(pass_speeds, n=10)[8]:.2f}")
    if pass_durations:
        print(f"Pass Duration (s): Mean={statistics.mean(pass_durations):.2f}, Median={statistics.median(pass_durations):.2f}")

    print(f"\nTotal Shots: {len(shots)}")
    on_target = sum(1 for s in shots if s['on_target'])
    print(f"Shots On Target: {on_target} ({on_target/len(shots)*100:.1f}%)")
    if shot_distances:
        print(f"Shot Distance (m): Mean={statistics.mean(shot_distances):.2f}, Median={statistics.median(shot_distances):.2f}, Min={min(shot_distances):.2f}, Max={max(shot_distances):.2f}")

def analyze_metrica(filepath):
    print(f"\n=== METRICA SPORTS ANALYSIS ({filepath}) ===")
    passes = 0
    shots = 0
    recoveries = 0
    tackles = 0
    ball_losts = 0
    pass_lengths_m = []

    # Metrica coordinates are 0..1 on 105 x 68m pitch
    with open(filepath, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            t = row.get('Type')
            if t == 'PASS':
                passes += 1
                try:
                    sx = float(row['Start X']) * 105.0
                    sy = float(row['Start Y']) * 68.0
                    ex = float(row['End X']) * 105.0
                    ey = float(row['End Y']) * 68.0
                    length = math.sqrt((ex - sx)**2 + (ey - sy)**2)
                    pass_lengths_m.append(length)
                except (ValueError, TypeError):
                    pass
            elif t == 'SHOT':
                shots += 1
            elif t == 'RECOVERY':
                recoveries += 1
            elif t == 'CHALLENGE':
                tackles += 1
            elif t == 'BALL LOST':
                ball_losts += 1

    print(f"Total Passes: {passes}")
    print(f"Total Shots: {shots}")
    print(f"Total Recoveries: {recoveries}")
    print(f"Total Challenges/Duels: {tackles}")
    print(f"Total Ball Losts: {ball_losts}")
    if pass_lengths_m:
        print(f"Metrica Pass Length (m): Mean={statistics.mean(pass_lengths_m):.2f}, Median={statistics.median(pass_lengths_m):.2f}, 10th%={statistics.quantiles(pass_lengths_m, n=10)[0]:.2f}, 90th%={statistics.quantiles(pass_lengths_m, n=10)[8]:.2f}")

if __name__ == '__main__':
    analyze_statsbomb('c:/Users/USER-PC/Desktop/FOOTBALL/data/cl_final_events_22912.json')
    analyze_metrica('c:/Users/USER-PC/Desktop/FOOTBALL/data/Sample_Game_1_RawEventsData.csv')
    analyze_metrica('c:/Users/USER-PC/Desktop/FOOTBALL/data/Sample_Game_2_RawEventsData.csv')
